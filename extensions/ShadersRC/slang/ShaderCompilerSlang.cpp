#include "ShaderCompilerSlang.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <ranges>
#include <algorithm>

#include <boost/regex.hpp>

#include <slang.h>
#include <slang-com-ptr.h>

#include "slang/external/spirv-tools/include/spirv-tools/libspirv.h"





namespace vkbase::ShadersRC
{
    constexpr int GLOBAL_SESSIONS_MAX = 4;
    std::vector<Slang::ComPtr<slang::IGlobalSession>> globalSessions;
    std::mutex globalSessionsMutex;
    int globalSessionCount = 0;


    Slang::ComPtr<slang::IGlobalSession> acquire_global_session()
    {
        Slang::ComPtr<slang::IGlobalSession> globalSession = nullptr;
        do
        {
            globalSessionsMutex.lock();
            if (!globalSessions.empty())
            {
                globalSession = globalSessions.back();
                globalSessions.pop_back();
            }
            bool create_global_session = !globalSession && (globalSessionCount < GLOBAL_SESSIONS_MAX);
            if (create_global_session)
                ++globalSessionCount;
            globalSessionsMutex.unlock();


            if (create_global_session)
            {
                SlangGlobalSessionDesc desc = {};
                desc.minLanguageVersion=SLANG_LANGUAGE_VERSION_LATEST;
                createGlobalSession(&desc, globalSession.writeRef());
                if (!globalSession)
                {
                    throw std::runtime_error("Failed to create Slang global session");
                }
                globalSessionsMutex.lock();
                std::cout << "Slang global session created " << std::endl;
                globalSessionsMutex.unlock();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        while (!globalSession);

        return globalSession;
    }

    void return_global_session(const Slang::ComPtr<slang::IGlobalSession>& session)
    {
        globalSessionsMutex.lock();
        globalSessions.push_back(session);
        globalSessionsMutex.unlock();
    }

    static std::vector<uint32_t> blobToSpirvVector(slang::IBlob* blob)
    {
        if (!blob)
            return {};
        const size_t byteSize = blob->getBufferSize();
        std::vector<uint32_t> out(byteSize / 4);
        std::memcpy(out.data(), blob->getBufferPointer(), byteSize);
        return out;
    }

    std::tuple<std::vector<uint32_t>,std::string> compile_internal(bool debug,CompilationParameters info, const std::string& sourceWithDefines, const Slang::ComPtr<slang::IGlobalSession>& globalSession, SlangStage entryPointStage)
    {
        std::vector<uint32_t> spirv;
        //compilation options
        std::vector<slang::CompilerOptionEntry> options;
        if (debug)
        {
            options.emplace_back(
                slang::CompilerOptionEntry
                {
                    .name = slang::CompilerOptionName::Optimization,
                    .value = slang::CompilerOptionValue
                    {
                        .kind = slang::CompilerOptionValueKind::Int,
                        .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_NONE,
                    }
                });

            options.emplace_back(
                slang::CompilerOptionEntry
                {
                    .name = slang::CompilerOptionName::PreserveParameters,
                    .value = slang::CompilerOptionValue
                    {
                        .kind = slang::CompilerOptionValueKind::Int,
                        .intValue0 = 1,
                    }
                });
        }
        else
        {
            options.emplace_back(
                slang::CompilerOptionEntry
                {
                    .name = slang::CompilerOptionName::Optimization,
                    .value = slang::CompilerOptionValue
                    {
                        .kind = slang::CompilerOptionValueKind::Int,
                        .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_MAXIMAL,
                    }
                });
        }
        options.emplace_back(
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::LanguageVersion,
                .value = slang::CompilerOptionValue
                {
                    .kind = slang::CompilerOptionValueKind::Int,
                    .intValue0 = SLANG_LANGUAGE_VERSION_LATEST,
                }
            });
        options.emplace_back(
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::MatrixLayoutRow,
                .value = slang::CompilerOptionValue
                {
                    .kind = slang::CompilerOptionValueKind::Int,
                    .intValue0 = 1,
                }
            });



        slang::SessionDesc sessionDesc{};

        slang::TargetDesc targetDesc{};
        targetDesc.flags |= SLANG_COMPILE_FLAG_NO_MANGLING;
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("spirv_1_6");
        sessionDesc.compilerOptionEntryCount = options.size();
        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        auto search_paths_view = std::ranges::views::transform(info.includePaths, [](const std::string& path) { return path.c_str(); });
        std::vector<const char*> searchPaths(search_paths_view.begin(), search_paths_view.end());
        sessionDesc.searchPaths = searchPaths.data();
        sessionDesc.searchPathCount = static_cast<SlangInt>(searchPaths.size());

        std::vector<slang::PreprocessorMacroDesc> preprocessorMacros;
        sessionDesc.preprocessorMacros = preprocessorMacros.data();
        sessionDesc.preprocessorMacroCount =static_cast<SlangInt>( preprocessorMacros.size() );

        Slang::ComPtr<slang::ISession> session;
        globalSession->createSession(sessionDesc, session.writeRef());

        Slang::ComPtr<slang::IBlob> diagnostics;
        std::string diagnostics_string;
        diagnostics.setNull();
        Slang::ComPtr<slang::IModule> module=nullptr;

        module=session->loadModuleFromSourceString(
            info.fileName.c_str(),
            info.fileName.c_str(),
            sourceWithDefines.c_str(),
            diagnostics.writeRef());

        if (diagnostics)
        {
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        }
        if (!module)
        {
            return {std::vector<uint32_t>{},std::string(diagnostics_string)};
        }

        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        diagnostics.setNull();
        module->findAndCheckEntryPoint(info.entryPointName.c_str(), entryPointStage, entryPoint.writeRef(),
                                       diagnostics.writeRef());
        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());


        //Composition
        slang::IComponentType* components[] = {module.get(), entryPoint.get()};
        Slang::ComPtr<slang::IComponentType> program;
        diagnostics.setNull();
        auto result = session->createCompositeComponentType(components, 2, program.writeRef(), diagnostics.writeRef());
        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        if (SLANG_FAILED(result))
        {
            return {std::vector<uint32_t>{},std::string(diagnostics_string)};
        }

        //Linking
        Slang::ComPtr<slang::IComponentType> linkedProgram;
        diagnostics.setNull();
        result = program->link(linkedProgram.writeRef(), diagnostics.writeRef());
        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        if (SLANG_FAILED(result))
        {
            return {std::vector<uint32_t>{},std::string(diagnostics_string)};
        }


        //Kernel Code
        int entryPointIndex = 0; // only one entry point
        int targetIndex = 0; // only one target
        Slang::ComPtr<slang::IBlob> kernelBlob;
        diagnostics.setNull();
        result = linkedProgram->getEntryPointCode(
            entryPointIndex,
            targetIndex,
            kernelBlob.writeRef(),
            diagnostics.writeRef());

        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        if (SLANG_FAILED(result))
        {
            return {std::vector<uint32_t>{},std::string(diagnostics_string)};
        }

        spirv = blobToSpirvVector(kernelBlob);
        return {spirv,diagnostics_string};
    }

    std::map<std::string,FileDiagnostics> parse_diagnostics(std::string& diagnostics,const std::string& mainFileName,int lines_offset,bool modify_lines)
    {
        std::map<std::string,FileDiagnostics> result;

        if (diagnostics.empty())
            return result;

        // Matches an entire error block
        // Group 1: Severity (error|warning)
        // Group 2: Error code (E30015)
        // Group 3: Main msg
        // Group 4: filename
        // Group 5: line number
        // Group 6: column
        // Group 7: source code
        // Group 8: error marker (^^^^ message)
        boost::regex diagnostic_pattern(
            R"((?:(error|warning|fatal error)(?:\[([^\]]+)\])?:\s*([^\n]*)\n)?(?:[ \t]*\n)*[ \t]*-->\s*(.+?):(\d+):(\d+)\s*\n[ \t]*\|\s*\n(?:[ \t]*\d+[ \t]*\|\s*([^\n]*)\n)?[ \t]*\|\s*([^\n]*?)\n[ \t]*-+')"
        );

        boost::sregex_iterator iter(diagnostics.begin(), diagnostics.end(), diagnostic_pattern);
        boost::sregex_iterator end_iter;

        for (; iter != end_iter; ++iter)
        {
            const auto& match = *iter;

            std::string severity = match[1].matched ? match[1].str() : "error";
            std::string error_code = match[2].matched ? match[2].str() : "";
            std::string msg_text = match[3].matched ? match[3].str() : "";

            std::string file_name = match[4].str();
            int line_num = std::stoi(match[5].str());
            int column_num = std::stoi(match[6].str());

            std::string source_code = match[7].matched ? match[7].str() : "";
            std::string marker_line = match[8].matched ? match[8].str() : "";

            if (file_name == mainFileName && modify_lines)
            {
                line_num -= lines_offset;
                line_num = std::max(0, line_num);
            }

            if (result.find(file_name) == result.end())
                result[file_name] = FileDiagnostics{};

            int pos = column_num;
            size_t caret_pos = marker_line.find('^');
            if (caret_pos == std::string::npos) caret_pos = marker_line.find('~');
            if (caret_pos != std::string::npos)
            {
                pos = static_cast<int>(caret_pos);
            }

            std::string full_message;
            if (!error_code.empty()) full_message += error_code + ": ";
            if (!msg_text.empty()) full_message += msg_text + "\n";

            // Extract inline message if there's no preceeding message text
            if (msg_text.empty() && !marker_line.empty()) {
                 size_t msg_start = marker_line.find_last_of("^~");
                 if (msg_start != std::string::npos) {
                     msg_start++;
                     while (msg_start < marker_line.length() && std::isspace(marker_line[msg_start])) msg_start++;
                     if (msg_start < marker_line.length()) {
                         full_message += marker_line.substr(msg_start) + "\n";
                     }
                 }
            }

            if (!source_code.empty()) full_message += source_code + "\n";
            if (!marker_line.empty()) full_message += marker_line;

            Message msg{pos, full_message};

            if (severity == "warning")
                result[file_name].warningLines[line_num] = msg;
            else
                result[file_name].errorLines[line_num] = msg;
        }

        return result;
    }

    std::map<std::string,FileDiagnostics> parse_diagnostics(const std::string& diagnostics)
    {
        std::string diag_copy = diagnostics;
        return parse_diagnostics(diag_copy, "", 0, false);
    }

    CompilationResult compileShaderSlang(const CompilationParameters& info)
    {
        using namespace slang;

        auto start_time= std::chrono::high_resolution_clock::now();

        std::string sourceWithDefines;
        for (const auto& [k, v] : info.defines)
        {
            sourceWithDefines += "#define " + k + " " + v + "\n";
        }
        sourceWithDefines += info.source;


        //Translate entry point stage
        SlangStage entryPointStage;
        switch (info.type)
        {
        case ShaderType::Vertex:
            entryPointStage = SLANG_STAGE_VERTEX;
            break;
        case ShaderType::Fragment:
            entryPointStage = SLANG_STAGE_FRAGMENT;
            break;
        case ShaderType::Compute:
            entryPointStage = SLANG_STAGE_COMPUTE;
            break;
        default:
            {
                throw std::runtime_error("Unsupported shader type");
            }
        }

        //Acquire global session
        Slang::ComPtr<slang::IGlobalSession> globalSession = acquire_global_session();


        CompilationResult compilation_result{.success = true};
        std::string diagnostics_text;
        if (info.compile_release)
        {
            auto  [release_spv,release_diagnostics] = compile_internal(false,info, sourceWithDefines, globalSession, entryPointStage);
            if (!release_spv.empty())
            {
                compilation_result.spirv=std::move(release_spv);
            }
            else
            {
                compilation_result.success=false;
            }
            diagnostics_text=std::move(release_diagnostics);

        }
        if (info.compile_debug)
        {
            auto  [debug_spv,debug_diagnostics] = compile_internal(true,info, sourceWithDefines, globalSession, entryPointStage);

            if (!debug_spv.empty())
            {
                compilation_result.spirv_debug=debug_spv;
            }
            else
            {
                compilation_result.success=false;
            }
            diagnostics_text+=debug_diagnostics;
        }


        return_global_session(globalSession); //return compiler session to the pool


        const int lines_offset = static_cast<int>(info.defines.size());
        if (info.parse_diagnostics)
            compilation_result.diagnostics = parse_diagnostics(diagnostics_text, info.fileName, lines_offset, true);
        compilation_result.diagnostics_text= std::move(diagnostics_text);

        auto time=std::chrono::high_resolution_clock::now()-start_time;
        compilation_result.compilation_time= static_cast<double>(time.count()) / 1000000000.0;
        return compilation_result;
    }
}
