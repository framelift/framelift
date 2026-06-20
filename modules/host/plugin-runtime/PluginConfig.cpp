#include "PluginConfig.h"

#include <filesystem>
#include <fstream>
#include <string>

void PluginConfig::Load(const std::string& path)
{
    states_.clear();

    std::ifstream file(path);
    if (!file)
    {
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#')
        {
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

        const std::string id = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);
        if (id.empty())
        {
            continue;
        }
        // Anything other than an explicit "disabled" reads as enabled.
        states_[id] = (value != "disabled");
    }
}

void PluginConfig::Save(const std::string& path) const
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

    std::ofstream out(path, std::ios::trunc);
    out << "# FrameLift plugin enablement. Set a plugin to 'disabled' to stop it loading.\n";
    out << "# Plugins not listed here default to enabled.\n";
    for (const auto& [id, enabled] : states_) // std::map iterates sorted by id
    {
        out << id << '=' << (enabled ? "enabled" : "disabled") << '\n';
    }
}

std::unordered_set<std::string> PluginConfig::DisabledIds() const
{
    std::unordered_set<std::string> out;
    for (const auto& [id, enabled] : states_)
    {
        if (!enabled)
        {
            out.insert(id);
        }
    }
    return out;
}
