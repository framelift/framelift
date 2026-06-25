#pragma once
#include "PackageMetadata.h"
#include <framelift/IModule.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class IPackage;
class QPluginLoader;
class QObject;

// Loads every package DLL present in a directory. A package is a Qt plugin: its root
// object is a QObject implementing IPackage (see sdk/include/framelift/IPackage.h),
// carrying Q_PLUGIN_METADATA whose JSON gives the package/module identity + ABI. The
// loader reads that metadata via QPluginLoader::metaData() — without instantiating —
// to ABI-check and resolve dependencies, then instantiates each enabled module (any
// module whose id is NOT in the disabled set) via IPackage::CreateModule.
class PackageLoader
{
public:
    // One instantiated module within a loaded package.
    struct LoadedModule
    {
        std::string moduleId;
        IModule* module;
        QObject* viewModel; // may be nullptr
        std::string qmlEntryUrl;
        int renderOrder;
    };

    struct LoadedPackage
    {
        std::string name;                      // package id / enabled-list entry
        std::string moduleFile;                // shipped package binary filename.
        std::unique_ptr<QPluginLoader> loader; // owns the plugin's root IPackage instance
        IPackage* package;                     // qobject_cast<IPackage*>(loader->instance())
        PackageMetadata meta;                  // owned copy of the embedded metadata
        std::vector<LoadedModule> modules;     // every module instantiated from this package
    };

    // Module identity copied out of a present-but-not-loaded package's metadata, so
    // the settings UI can list and re-enable individual modules even when the package
    // is not loaded.
    struct AvailableModule
    {
        std::string id;
        std::string name;
        std::string description;
    };

    struct AvailablePackage
    {
        std::string packageId;
        std::string moduleFile;
        std::string displayName; // metadata name, or moduleFile when metadata is unreadable
        int version[3] = {0, 0, 0};
        std::string publisher;
        std::string description;
        std::vector<AvailableModule> modules;
    };

    PackageLoader();
    // Destroys every instantiated module via IPackage::DestroyModule and unloads each plugin.
    ~PackageLoader();

    // Scans packages/ for shared libraries and loads every ABI-compatible package.
    // Within each, every module whose id is NOT in `disabledModules` is instantiated.
    // Dependencies and load order are resolved from embedded package/module metadata
    // before any module object is created. Does NOT call Install(); the caller does
    // that via Registry().Add(module, ctx).
    void LoadAll(const std::string& modulesDir, const std::unordered_set<std::string>& disabledModules = {});

    const std::vector<LoadedPackage>& Packages() const
    {
        return packages_;
    }

    // Discover every package binary present in modulesDir (with full identity and its
    // module list), by reading embedded metadata only (no instantiation), so the
    // settings UI can list and re-enable packages/modules that are disabled or failed.
    static std::vector<AvailablePackage> DiscoverAvailable(const std::string& modulesDir);

private:
    std::vector<LoadedPackage> packages_;
};
