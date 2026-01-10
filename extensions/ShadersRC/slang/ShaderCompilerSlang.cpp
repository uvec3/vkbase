#include "ShaderCompilerSlang.hpp"

#include <iostream>
#include <mutex>
#include <slang.h>
#include <slang-com-ptr.h>
#include <stdexcept>
#include <string>
#include <vector>
#include<spirv-tools/libspirv.h>

namespace vkbase::ShadersRC
{
    constexpr int GLOBAL_SESSIONS_MAX = 8;
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
                createGlobalSession(&desc, globalSession.writeRef());
                globalSessionsMutex.lock();
                std::cout << "Global session created " << std::endl;
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
        if (!blob) return {};
        const size_t byteSize = blob->getBufferSize();
        std::vector<uint32_t> out(byteSize / 4);
        std::memcpy(out.data(), blob->getBufferPointer(), byteSize);
        return out;
    }

    std::vector<uint32_t>
    compileShaderSlang(const std::string& source, const std::string& fileName, ShaderType type,
                       const ::std::string& entryPointName, const std::map<std::string, std::string>& defines,
                       bool debug, std::string& diagnostics_string)
    {

        std::string sourceWithDefines;
        for (const auto& [k, v] : defines)
        {
            sourceWithDefines += "#define " + k + " " + v + "\n";
        }
        sourceWithDefines += source;

        using namespace slang;

        //Acquire global session
        Slang::ComPtr<slang::IGlobalSession> globalSession = acquire_global_session();

        //Translate entry point stage
        SlangStage entryPointStage;
        switch (type)
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
                return_global_session(globalSession);
                throw std::runtime_error("Unsupported shader type for Slang compilation.");
            }
        }


        //compilation options
        std::vector<slang::CompilerOptionEntry> options;
        if (debug)
        {
            options.emplace_back(
                CompilerOptionEntry
                {
                    .name = CompilerOptionName::Optimization,
                    .value = CompilerOptionValue
                    {
                        .kind = CompilerOptionValueKind::Int,
                        .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_NONE,
                    }
                });

            options.emplace_back(
                CompilerOptionEntry
                {
                    .name = CompilerOptionName::PreserveParameters,
                    .value = CompilerOptionValue
                    {
                        .kind = CompilerOptionValueKind::Int,
                        .intValue0 = 1,
                    }
                });
        }
        else
        {
            options.emplace_back(
                CompilerOptionEntry
                {
                    .name = CompilerOptionName::Optimization,
                    .value = CompilerOptionValue
                    {
                        .kind = CompilerOptionValueKind::Int,
                        .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_MAXIMAL,
                    }
                });
        }
        options.emplace_back(
            CompilerOptionEntry
            {
                .name = CompilerOptionName::LanguageVersion,
                .value = CompilerOptionValue
                {
                    .kind = CompilerOptionValueKind::Int,
                    .intValue0 = SLANG_LANGUAGE_VERSION_LATEST,
                }
            });
        options.emplace_back(
            CompilerOptionEntry
            {
                .name = CompilerOptionName::MatrixLayoutRow,
                .value = CompilerOptionValue
                {
                    .kind = CompilerOptionValueKind::Int,
                    .intValue0 = 1,
                }
            });

        options.emplace_back(
            CompilerOptionEntry
            {
                .name = CompilerOptionName::EmitSpirvMethod,
                .value = CompilerOptionValue
                {
                    .kind = CompilerOptionValueKind::Int,
                    .intValue0 = SLANG_EMIT_SPIRV_VIA_GLSL,
                }
            });


        options.emplace_back(
            CompilerOptionEntry
            {
                .name = CompilerOptionName::EmitSpirvViaGLSL,
                .value = CompilerOptionValue
                {
                    .kind = CompilerOptionValueKind::Int,
                    .intValue0 = 1,
                }
            });

        options.emplace_back(
            CompilerOptionEntry
            {
                .name = CompilerOptionName::EmitSpirvDirectly,
                .value = CompilerOptionValue
                {
                    .kind = CompilerOptionValueKind::Int,
                    .intValue0 = 0,
                }
            });

        SessionDesc sessionDesc;
        /* ... fill in `sessionDesc` ... */


        TargetDesc targetDesc;
        targetDesc.flags |= SLANG_COMPILE_FLAG_NO_MANGLING;
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = globalSession->findProfile("spirv_1_6");
        sessionDesc.compilerOptionEntryCount = options.size();
        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        // const char* searchPaths[] = { };
        // sessionDesc.searchPaths = searchPaths;
        sessionDesc.searchPathCount = 0;


        std::vector<PreprocessorMacroDesc> preprocessorMacros;
        // for (const auto& [k, v] : defines)
        // {
        //     PreprocessorMacroDesc macro;
        //     macro.name = k.c_str();
        //     macro.value = v.c_str();
        //     preprocessorMacros.push_back(macro);
        // }

        sessionDesc.preprocessorMacros = preprocessorMacros.data();
        sessionDesc.preprocessorMacroCount = preprocessorMacros.size();

        Slang::ComPtr<ISession> session;
        globalSession->createSession(sessionDesc, session.writeRef());

        Slang::ComPtr<IBlob> diagnostics;
        Slang::ComPtr<IModule> module(session->loadModuleFromSourceString(fileName.c_str(), fileName.c_str(),
                                                                          sourceWithDefines.c_str(), diagnostics.writeRef()));
        if (diagnostics)
        {
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        }
        if (!module)
        {
            return_global_session(globalSession);
            throw std::runtime_error(std::string(diagnostics_string));
        }

        Slang::ComPtr<IEntryPoint> computeEntryPoint;
        module->findAndCheckEntryPoint(entryPointName.c_str(), entryPointStage, computeEntryPoint.writeRef(),
                                       diagnostics.writeRef());
        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());


        //Composition
        IComponentType* components[] = {module, computeEntryPoint};
        Slang::ComPtr<IComponentType> program;
        auto result = session->createCompositeComponentType(components, 2, program.writeRef(), diagnostics.writeRef());
        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        if (SLANG_FAILED(result))
        {
            return_global_session(globalSession);
            throw std::runtime_error(diagnostics_string);
        }

        //Linking
        Slang::ComPtr<IComponentType> linkedProgram;
        result = program->link(linkedProgram.writeRef(), diagnostics.writeRef());
        if (diagnostics)
            diagnostics_string += std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                              diagnostics->getBufferSize());
        if (SLANG_FAILED(result))
        {
            return_global_session(globalSession);
            throw std::runtime_error(diagnostics_string);
        }


        //Kernel Code
        int entryPointIndex = 0; // only one entry point
        int targetIndex = 0; // only one target
        Slang::ComPtr<IBlob> kernelBlob;
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
            return_global_session(globalSession);
            throw std::runtime_error(diagnostics_string);
        }

        auto spirv = blobToSpirvVector(kernelBlob);
        return_global_session(globalSession); //return session to the pool
        return spirv;
    }
}
