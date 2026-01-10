#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "../ShaderType.hpp"
#include <cstdint>

namespace vkbase::ShadersRC {



    [[maybe_unused]] std::vector<uint32_t>compileShaderSlang(const std::string &source, const std::string &fileName, ShaderType type,
                                                             const ::std::string &entryPointName, const std::map<std::string, std::string> &defines,
                                                             bool debug, std::string& diagnostics);



}
