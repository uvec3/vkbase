#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <functional>
#include <optional>

namespace vkbase::rc
{
    typedef std::vector<uint32_t> Spirv;

    enum class ShaderType {
        Vertex,
        Fragment,
        Compute,
        Geometry,
        TessControl,
        TessEvaluation
    };


    // typedef std::string (*FnLoadFile)(const std::string& path);
    typedef std::function<std::optional<std::string>(const std::string& path)> FnLoadFile;


    struct CompilationParameters
    {
        std::string source;
        ShaderType type=ShaderType::Compute;
        std::string fileName="shader";
        std::string entryPointName="main";
        std::string target="SPIRV";
        std::map<std::string, std::string> defines;
        std::vector<std::string> includePaths;

        std::optional<FnLoadFile> loadFileFn=std::nullopt;

        bool compile_release=false;
        bool compile_debug=false;
        bool parse_diagnostics=true;

        bool source_is_glsl=false;
    };

    struct Message
    {
        int pos;
        std::string message;
    };

    struct FileDiagnostics
    {
        std::map<int, Message> errorLines;
        std::map<int, Message> warningLines;
    };

    struct CompilationResult
    {
        bool success;
        std::vector<uint32_t> spirv;
        std::vector<uint32_t> spirv_debug;
        std::string text;
        std::string diagnostics_text;
        std::map<std::string, FileDiagnostics> diagnostics;
        double compilation_time;
    };
}