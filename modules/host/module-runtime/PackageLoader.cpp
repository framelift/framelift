#include "PackageLoader.h"
#include "PackageResolver.h"
#include <chrono>
#include <filesystem>
#include <framelift/IPackage.h>
#include <framelift/Log.h>
#include <framelift/ModuleABI.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QtCore/QJsonObject>
#include <QtCore/QPluginLoader>
#include <QtCore/QString>

Log::SinkFn HostLogSink();

namespace
{
#ifdef _WIN32
constexpr const char* kPackageExt = ".dll";
#else
constexpr const char* kPackageExt = ".so";
#endif

struct PackageBinary
{
    std::string moduleFile;
    std::string path;
};

struct PackageCandidate
{
    PackageBinary binary;
    std::unique_ptr<QPluginLoader> loader;
    PackageMetadata meta;
};

std::vector<PackageBinary> DiscoverPackageBinaries(const std::string& modulesDir)
{
    namespace fs = std::filesystem;
    std::vector<PackageBinary> out;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(modulesDir, ec))
    {
        std::error_code fec;
        if (!entry.is_regular_file(fec) || entry.path().extension() != kPackageExt)
        {
            continue;
        }

        out.push_back({entry.path().filename().string(), entry.path().string()});
    }

    return out;
}

// Read a plugin's embedded Q_PLUGIN_METADATA without loading the plugin: metaData()
// works on an unloaded QPluginLoader, and the author JSON sits under "MetaData".
PackageMetadata ReadPackageMetadata(QPluginLoader& loader)
{
    return ParsePackageMetadata(loader.metaData().value("MetaData").toObject());
}

bool AbiCompatible(const PackageMetadata& meta)
{
    return meta.valid && FrameLiftAbiCompatible(meta.abiVersion, FRAMELIFT_ABI_VERSION);
}
} // namespace

PackageLoader::PackageLoader() = default;

void PackageLoader::LoadAll(const std::string& modulesDir, const std::unordered_set<std::string>& disabledModules)
{
    using Clock = std::chrono::steady_clock;
    const auto loadStart = Clock::now();

    // Read metadata for every package present in the directory (dedupe by id). No
    // plugin code runs here — metaData() reads the embedded JSON only.
    std::vector<PackageCandidate> candidates;
    std::unordered_map<std::string, std::size_t> seenIds;
    for (const auto& binary : DiscoverPackageBinaries(modulesDir))
    {
        auto loader = std::make_unique<QPluginLoader>(QString::fromStdString(binary.path));
        PackageMetadata meta = ReadPackageMetadata(*loader);
        if (!meta.valid)
        {
            Log::Warn("Package '{}': missing/invalid metadata - rebuild against current SDK", binary.moduleFile);
            continue;
        }
        if (!AbiCompatible(meta))
        {
            Log::Warn(
                "Package '{}' v{}.{}.{}: ABI version {} incompatible with host version {} - rebuild against current "
                "SDK",
                meta.packageId, meta.version[0], meta.version[1], meta.version[2], meta.abiVersion,
                FRAMELIFT_ABI_VERSION
            );
            continue;
        }
        if (seenIds.contains(meta.packageId))
        {
            Log::Warn("Package '{}': duplicate package id - skipped", meta.packageId);
            continue;
        }
        seenIds.emplace(meta.packageId, candidates.size());
        candidates.push_back(PackageCandidate{binary, std::move(loader), std::move(meta)});
    }

    // Resolve dependencies over every discovered package, then load the accepted
    // ones in dependency order (providers before consumers).
    std::vector<PackageResolveCandidate> resolveCandidates;
    resolveCandidates.reserve(candidates.size());
    for (const auto& candidate : candidates)
    {
        resolveCandidates.push_back({&candidate.meta});
    }

    const std::vector<PackageResolveDecision> decisions =
        ResolvePackages(resolveCandidates, FrameLiftCurrentPlatformId());

    std::vector<PackageResolveCandidate> acceptedResolve;
    std::vector<std::size_t> acceptedIndex;
    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        if (decisions[i].accepted)
        {
            acceptedResolve.push_back({&candidates[i].meta});
            acceptedIndex.push_back(i);
        }
        else
        {
            Log::Warn("Package '{}': {} - skipped", candidates[i].meta.packageId, decisions[i].reason);
        }
    }

    // Diagnose colliding providers: two accepted packages declaring the same module
    // id or feature. We keep first-wins (load order decides the winner) but warn so
    // the collision is visible rather than silent.
    {
        std::unordered_map<std::string, std::string> firstProvider; // token -> package id
        const auto note = [&](const std::string& token, const char* kind, const std::string& owner)
        {
            if (token.empty())
            {
                return;
            }
            const auto [it, inserted] = firstProvider.emplace(token, owner);
            if (!inserted)
            {
                Log::Warn(
                    "Package '{}': {} '{}' already provided by '{}' - keeping the first provider", owner, kind, token,
                    it->second
                );
            }
        };
        for (const auto& accepted : acceptedResolve)
        {
            const PackageMetadata& meta = *accepted.meta;
            for (const ModuleMetadata& module : meta.modules)
            {
                note(module.id, "module", meta.packageId);
                for (const std::string& feature : module.providesFeatures)
                {
                    note(feature, "feature", meta.packageId);
                }
            }
        }
    }

    for (const std::size_t orderIdx : OrderPackages(acceptedResolve))
    {
        PackageCandidate& candidate = candidates[acceptedIndex[orderIdx]];
        const PackageMetadata& meta = candidate.meta;

        const auto packageStart = Clock::now();

        QObject* const root = candidate.loader->instance();
        if (!root)
        {
            Log::Warn(
                "Package '{}': instance() failed ({}) - skipped", meta.packageId,
                candidate.loader->errorString().toStdString()
            );
            continue;
        }
        IPackage* const package = qobject_cast<IPackage*>(root);
        if (!package)
        {
            Log::Warn("Package '{}': root object is not an IPackage - skipped", meta.packageId);
            candidate.loader->unload();
            continue;
        }

        package->SetLogSink(HostLogSink());

        // Instantiate every module the package carries whose id the user hasn't
        // disabled. A package may end up loading some of its modules and not others.
        std::vector<LoadedModule> loadedModules;
        for (const ModuleMetadata& module : meta.modules)
        {
            if (disabledModules.contains(module.id))
            {
                Log::Info("Module '{}': disabled by user - skipped", module.id);
                continue;
            }

            IModule* const instance = package->CreateModule(module.id.c_str());
            if (!instance)
            {
                Log::Warn("Module '{}': CreateModule() returned nullptr - skipped", module.id);
                continue;
            }

            const int order = package->RenderOrder(module.id.c_str());
            QObject* const viewModel = package->GetViewModel(module.id.c_str(), instance);
            const char* const qmlEntry = package->QmlEntryUrl(module.id.c_str());
            loadedModules.push_back(
                {module.id, instance, viewModel, qmlEntry ? std::string(qmlEntry) : std::string{}, order}
            );
        }

        if (loadedModules.empty())
        {
            // Every module disabled (or each failed to construct): the plugin stays
            // loaded for nothing, so drop it. It still shows in the settings UI via
            // DiscoverAvailable so the user can re-enable a module.
            Log::Info("Package '{}': no enabled modules - not loaded", meta.packageId);
            candidate.loader->unload();
            continue;
        }

        const std::string by = meta.publisher.empty() ? std::string() : std::string(" by ") + meta.publisher;
        const std::size_t moduleCount = meta.modules.size();
        const std::size_t loadedCount = loadedModules.size();
        packages_.push_back(
            {meta.packageId, candidate.binary.moduleFile, std::move(candidate.loader), package, candidate.meta,
             std::move(loadedModules)}
        );

        const double packageMs = std::chrono::duration<double, std::milli>(Clock::now() - packageStart).count();
        Log::Info(
            "Package '{}' v{}.{}.{}{} loaded (abi version {}, {} of {} module(s), {:.1f} ms)", meta.packageId,
            meta.version[0], meta.version[1], meta.version[2], by, meta.abiVersion, loadedCount, moduleCount, packageMs
        );
    }

    const double totalMs = std::chrono::duration<double, std::milli>(Clock::now() - loadStart).count();
    Log::Info("Loaded {} package(s) in {:.1f} ms", packages_.size(), totalMs);
}

std::vector<PackageLoader::AvailablePackage> PackageLoader::DiscoverAvailable(const std::string& modulesDir)
{
    std::vector<AvailablePackage> out;
    for (const auto& binary : DiscoverPackageBinaries(modulesDir))
    {
        QPluginLoader loader(QString::fromStdString(binary.path));
        const PackageMetadata meta = ReadPackageMetadata(loader);
        if (meta.valid && AbiCompatible(meta) && !meta.packageId.empty())
        {
            AvailablePackage pkg;
            pkg.packageId = meta.packageId;
            pkg.moduleFile = binary.moduleFile;
            pkg.displayName = meta.name.empty() ? meta.packageId : meta.name;
            pkg.version[0] = meta.version[0];
            pkg.version[1] = meta.version[1];
            pkg.version[2] = meta.version[2];
            pkg.publisher = meta.publisher;
            pkg.description = meta.description;
            for (const ModuleMetadata& module : meta.modules)
            {
                pkg.modules.push_back({module.id, module.name, module.description});
            }
            out.push_back(std::move(pkg));
        }
        else
        {
            out.push_back({binary.moduleFile, binary.moduleFile, binary.moduleFile});
        }
    }
    return out;
}

PackageLoader::~PackageLoader()
{
    for (auto& p : packages_)
    {
        for (const auto& m : p.modules)
        {
            p.package->DestroyModule(m.module);
        }
        if (p.loader)
        {
            p.loader->unload();
        }
    }
}
