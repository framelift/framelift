#pragma once
#include <string>
#include <vector>

class QJsonObject;

// Host-side, owned representation of a package's embedded Q_PLUGIN_METADATA JSON.
// Parsed from QPluginLoader::metaData()["MetaData"] — no plugin code is run to read
// it. Replaces the old compiled-in FrameLiftPackageInfo/FrameLiftModuleInfo POD that
// crossed the framelift_module_info() boundary; metadata now travels as JSON, so the
// host owns std::string/std::vector copies instead of pointers into a loaded DLL.

struct ModuleMetadata
{
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> providesFeatures;
    std::vector<std::string> requiresModules;
    std::vector<std::string> requiresFeatures;
    std::vector<std::string> optionalModules;
    std::vector<std::string> optionalFeatures;
    std::vector<std::string> platforms; // empty means every platform
};

struct PackageMetadata
{
    bool valid = false; // false if the JSON was missing/unparseable
    int abiVersion = 0;
    std::string packageId;
    std::string moduleFile;
    std::string name;
    int version[3] = {0, 0, 0};
    std::string publisher;
    std::string description;
    std::vector<ModuleMetadata> modules;
};

// Parse the "MetaData" object from a plugin's QPluginLoader::metaData(). On a missing
// or malformed object the result has valid=false (and should be skipped by the loader).
PackageMetadata ParsePackageMetadata(const QJsonObject& metaData);
