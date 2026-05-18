#include <filesystem>
#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(vkbase_assets);

namespace vkbase::assets
{
    std::vector<std::pair<std::string_view, std::string_view>> assetsFrom(const std::string& path)
    {
        auto fs = cmrc::vkbase_assets::get_filesystem();
        std::vector<std::pair<std::string_view, std::string_view>> results;

        std::function<void(const std::string&)> visit = [&](const std::string& dir)
        {

            for (const auto& entry : fs.iterate_directory(dir))
            {
                std::string entry_path = dir.empty() ? entry.filename() : dir + "/" + entry.filename();
                if (entry.is_directory())
                {
                    visit(entry_path);
                }
                else
                {
                    auto file = fs.open(entry_path);
                    results.emplace_back(entry_path, std::string_view(file.begin(), file.end()));
                }
            }
        };

        visit(path);
        return results;
    }

    std::string_view get(const std::string& path)
    {
        auto fs = cmrc::vkbase_assets::get_filesystem();
        if (!fs.exists(path))
            return "";
        auto file = fs.open(path);
        return {file.cbegin(), file.cend()};
    }

    cmrc::directory_iterator directory(const std::string& path)
    {
        auto fs = cmrc::vkbase_assets::get_filesystem();
        auto dir_it=fs.iterate_directory(path);
        return dir_it;
    }
}
