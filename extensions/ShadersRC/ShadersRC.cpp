#include "ShadersRC.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <memory>
#include <cstdlib>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "glslang/MachineIndependent/localintermediate.h"
#include "slang/slang/external/spirv-tools/include/spirv-tools/libspirv.hpp"
#include  "slang/slang/external/spirv-tools/include/spirv-tools/libspirv.h"
#include  "slang/slang/external/spirv-tools/include/spirv-tools/optimizer.hpp"

namespace vkbase::rc
{
    namespace
    {
        void ensure_glslang_initialized()
        {
            static std::once_flag init_flag;
            std::call_once(init_flag, []() {
                glslang::InitializeProcess();
                std::atexit(glslang::FinalizeProcess);
            });
        }

        EShLanguage to_glslang_stage(ShaderType type)
        {
            switch (type)
            {
            case ShaderType::Vertex:
                return EShLangVertex;
            case ShaderType::Fragment:
                return EShLangFragment;
            case ShaderType::Compute:
                return EShLangCompute;
            case ShaderType::Geometry:
                return EShLangGeometry;
            case ShaderType::TessControl:
                return EShLangTessControl;
            case ShaderType::TessEvaluation:
                return EShLangTessEvaluation;
            default:
                break;
            }
            throw std::runtime_error("Unsupported shader type.");
        }

        std::string build_preamble(const CompilationParameters& info)
        {
            if (info.defines.empty())
                return {};
            std::string preamble;
            for (const auto& [key, value] : info.defines)
            {
                preamble += "#define ";
                preamble += key;
                if (!value.empty())
                {
                    preamble += " ";
                    preamble += value;
                }
                preamble += "\n";
            }
            return preamble;
        }

        std::optional<std::string> load_file_from_disk(const std::filesystem::path& path)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream)
                return std::nullopt;
            std::string contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            return contents;
        }

        class GlslangIncluder : public glslang::TShader::Includer
        {
        public:
            explicit GlslangIncluder(const CompilationParameters& info)
                : info(info)
            {
            }

            IncludeResult* includeLocal(const char* requested_source,
                                        const char* includer_name,
                                        size_t include_depth) override
            {
                return include_from(requested_source, includer_name, include_depth);
            }

            IncludeResult* includeSystem(const char* requested_source,
                                         const char* includer_name,
                                         size_t include_depth) override
            {
                return include_from(requested_source, includer_name, include_depth);
            }

            void releaseInclude(IncludeResult* result) override
            {
                delete result;
            }

        private:
            const CompilationParameters& info;
            std::vector<std::unique_ptr<std::string>> owned_sources;

            IncludeResult* include_from(const char* requested_source,
                                        const char* includer_name,
                                        size_t)
            {
                std::filesystem::path requested_path(requested_source);
                std::vector<std::filesystem::path> candidates;
                if (includer_name && *includer_name)
                {
                    std::filesystem::path includer_path(includer_name);
                    candidates.push_back(includer_path.parent_path() / requested_path);
                }
                for (const auto& include_path : info.includePaths)
                    candidates.push_back(std::filesystem::path(include_path) / requested_path);
                if (candidates.empty())
                    candidates.push_back(requested_path);

                for (const auto& candidate : candidates)
                {
                    std::optional<std::string> contents;
                    if (info.loadFileFn.has_value())
                        contents = info.loadFileFn.value()(candidate.string());
                    else
                        contents = load_file_from_disk(candidate);

                    if (contents.has_value())
                    {
                        owned_sources.emplace_back(std::make_unique<std::string>(std::move(contents.value())));
                        auto& stored = *owned_sources.back();
                        return new IncludeResult(candidate.string(), stored.c_str(), stored.size(), nullptr);
                    }
                }

                return nullptr;
            }
        };
    }
    std::string spirv_to_text(const std::vector<uint32_t> &spirv_code, bool for_print)
    {
        //use spvBinaryToText from SPIRV-Tools
        spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_6);
        spv_text text = nullptr;


        spv_diagnostic diagnostic;
        auto flags= for_print ? SPV_BINARY_TO_TEXT_OPTION_PRINT : SPV_BINARY_TO_TEXT_OPTION_NONE;
        spv_result_t error =
                spvBinaryToText(context, spirv_code.data(), spirv_code.size(), SPV_BINARY_TO_TEXT_OPTION_NONE, &text,
                                &diagnostic);

        if (error != SPV_SUCCESS)
            throw std::runtime_error(std::string(diagnostic->error));
        return std::string{text->str, text->length};
    }

    std::vector<uint32_t> text_to_spirv(const std::string &text)
    {
        spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_6);
        spv_binary binary = nullptr;
        spv_diagnostic diagnostic;
        spv_result_t error =
                spvTextToBinary(context, text.c_str(), text.size(), &binary, &diagnostic);

        if (error != SPV_SUCCESS)
            throw std::runtime_error(std::string(diagnostic->error));

        std::vector<uint32_t> spirv_code(binary->code, binary->code + binary->wordCount);
        spvBinaryDestroy(binary);
        return spirv_code;
    }

    std::vector<uint32_t> optimize_spirv(const std::vector<uint32_t> &spirv_code)
    {
        spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_4);
        optimizer.RegisterPerformancePasses(true);
        optimizer.SetMessageConsumer([](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
            std::string level_str;
            switch (level) {
                case SPV_MSG_FATAL: level_str = "FATAL"; break;
                case SPV_MSG_INTERNAL_ERROR: level_str = "INTERNAL_ERROR"; break;
                case SPV_MSG_ERROR: level_str = "ERROR"; break;
                case SPV_MSG_WARNING: level_str = "WARNING"; break;
                case SPV_MSG_INFO: level_str = "INFO"; break;
                default: level_str = "UNKNOWN"; break;
            }
            std::cerr << "[" << level_str << "] " <<": " << message << std::endl;
        });

        std::vector<uint32_t> optimized_spirv;
        if (!optimizer.Run(spirv_code.data(), spirv_code.size(), &optimized_spirv)) {
            std::cerr<<"Failed to optimize SPIR-V";
            return spirv_code;
        }

        return optimized_spirv;
    }


    Spirv compile_glsl(const CompilationParameters& info)
    {
        ensure_glslang_initialized();

        const auto stage = to_glslang_stage(info.type);
        const char* source_cstr = info.source.c_str();
        glslang::TShader shader(stage);
        shader.setStrings(&source_cstr, 1);
        shader.setEntryPoint(info.entryPointName.c_str());
        shader.setSourceEntryPoint(info.entryPointName.c_str());
        shader.setPreamble(build_preamble(info).c_str());
        shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
        shader.setAutoMapLocations(true);

        EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
        GlslangIncluder includer(info);

        const TBuiltInResource* resources = GetDefaultResources();
        if (!shader.parse(resources, 100, false, messages, includer))
        {
            std::cerr << "GLSLang parse error: " << shader.getInfoLog() << std::endl;
            std::cerr << shader.getInfoDebugLog() << std::endl;
            return {};
        }

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(messages))
        {
            std::cerr << "GLSLang link error: " << program.getInfoLog() << std::endl;
            std::cerr << program.getInfoDebugLog() << std::endl;
            return {};
        }

        program.mapIO();



        glslang::TIntermediate* intermediate = program.getIntermediate(stage);
        if (!intermediate)
        {
            std::cerr << "GLSLang error: missing intermediate." << std::endl;
            return {};
        }

        intermediate->setAutoMapLocations(true);

        glslang::SpvOptions spv_options;
        spv_options.disableOptimizer = info.compile_debug;
        spv_options.generateDebugInfo = info.compile_debug;
        spv_options.stripDebugInfo = false;


        std::vector<uint32_t> spirv;
        glslang::GlslangToSpv(*intermediate, spirv, &spv_options);
        return spirv;
    }

}
