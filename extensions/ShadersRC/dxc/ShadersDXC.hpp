#pragma once
#include <vector>
#include <string>
#include <map>
#include <vulkan/vulkan.h>
#include "../ShaderType.hpp"



namespace vkbase::ShadersRC {
    [[maybe_unused]] std::vector<uint32_t>
    compileShaderDXC(const std::string &source, const std::string &fileName, ShaderType type,
                     const ::std::string &entryPoint, const std::map<std::string, std::string> &defines,
                     bool debug = false);
}
