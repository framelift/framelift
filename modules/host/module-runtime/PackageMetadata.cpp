#include "PackageMetadata.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

namespace
{
std::string ToStd(const QJsonValue& v)
{
    return v.isString() ? v.toString().toStdString() : std::string();
}

std::vector<std::string> ToStringList(const QJsonValue& v)
{
    std::vector<std::string> out;
    if (v.isArray())
    {
        const QJsonArray arr = v.toArray();
        out.reserve(static_cast<std::size_t>(arr.size()));
        for (const QJsonValue& item : arr)
        {
            if (item.isString())
            {
                out.push_back(item.toString().toStdString());
            }
        }
    }
    return out;
}
} // namespace

PackageMetadata ParsePackageMetadata(const QJsonObject& metaData)
{
    PackageMetadata meta;

    const QJsonValue modulesVal = metaData.value("modules");
    if (!metaData.contains("packageId") || !modulesVal.isArray())
    {
        return meta; // valid stays false
    }

    meta.abiVersion = metaData.value("abi").toInt(0);
    meta.packageId = ToStd(metaData.value("packageId"));
    meta.moduleFile = ToStd(metaData.value("moduleFile"));
    meta.name = ToStd(metaData.value("name"));
    meta.publisher = ToStd(metaData.value("publisher"));
    meta.description = ToStd(metaData.value("description"));

    const QJsonArray versionArr = metaData.value("version").toArray();
    for (int i = 0; i < 3 && i < versionArr.size(); ++i)
    {
        meta.version[i] = versionArr.at(i).toInt(0);
    }

    const QJsonArray modulesArr = modulesVal.toArray();
    meta.modules.reserve(static_cast<std::size_t>(modulesArr.size()));
    for (const QJsonValue& moduleVal : modulesArr)
    {
        const QJsonObject moduleObj = moduleVal.toObject();
        ModuleMetadata module;
        module.id = ToStd(moduleObj.value("id"));
        module.name = ToStd(moduleObj.value("name"));
        module.description = ToStd(moduleObj.value("description"));
        module.providesFeatures = ToStringList(moduleObj.value("providesFeatures"));
        module.requiresModules = ToStringList(moduleObj.value("requiresModules"));
        module.requiresFeatures = ToStringList(moduleObj.value("requiresFeatures"));
        module.optionalModules = ToStringList(moduleObj.value("optionalModules"));
        module.optionalFeatures = ToStringList(moduleObj.value("optionalFeatures"));
        module.platforms = ToStringList(moduleObj.value("platforms"));
        meta.modules.push_back(std::move(module));
    }

    meta.valid = true;
    return meta;
}
