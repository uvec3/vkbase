#pragma once
#include <string>
#include <cmrc/cmrc.hpp>

namespace vkbase::assets
{
    std::vector<std::pair<std::string_view,std::string_view>> assetsFrom(const std::string& path);
    std::string_view get(const std::string& path);
    cmrc::directory_iterator directory(const std::string& path);

}
