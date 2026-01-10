#include "ShadersDXC.hpp"

#include <codecvt>
#include <shaderc/shaderc.hpp>
#include <locale>
#include <map>

#include <stdexcept>
#include <string>
#include <vector>


#ifdef _WIN32
    #include <windows.h>
#elif
    #include "dxc/WinAdapter.h"
#endif
#include "dxcapi.h"
namespace vkbase::ShadersRC
{
    template<typename T>
        struct ComPtrLite {
        T* ptr = nullptr;
        ComPtrLite() = default;
        ComPtrLite(T* p): ptr(p) {}
        ~ComPtrLite(){ if(ptr) ptr->Release(); }
        T** operator&() { return &ptr; } // for APIs that write a T**
        T* operator->() const { return ptr; }
        operator T*() const { return ptr; }
        void Attach(T* p){ if(ptr) ptr->Release(); ptr = p; }
        T* Detach(){ T* p = ptr; ptr = nullptr; return p; }
    };

    std::vector<uint32_t> compileShaderDXC(const std::string &source, const std::string &fileName, ShaderType type,
                                           const std::string &entryPoint,
                                           const std::map<std::string, std::string> &defines, bool debug)
    {
        HRESULT hres;

        // Initialize DXC library
        ComPtrLite < IDxcLibrary > library;
        hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
        if (FAILED(hres))
        {
            throw std::runtime_error("Could not init DXC Library");
        }

        // Initialize DXC compiler
        ComPtrLite < IDxcCompiler3 > compiler;
        hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(hres))
        {
            throw std::runtime_error("Could not init DXC Compiler");
        }

        // Initialize DXC utility
        ComPtrLite < IDxcUtils > utils;
        hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if (FAILED(hres))
        {
            throw std::runtime_error("Could not init DXC Utiliy");
        }

        // Create blob from in-memory source string
        ComPtrLite<IDxcBlobEncoding> sourceBlob;
        hres = library->CreateBlobWithEncodingOnHeapCopy(
            source.data(),
            static_cast<UINT32>(source.size()),
            DXC_CP_UTF8,
            &sourceBlob);
        if (FAILED(hres)) {
            throw std::runtime_error("Could not create shader blob from source");
        }

        // Select target profile
        LPCWSTR targetProfile{};
        switch (type)
        {
            case ShaderType::Vertex:
                targetProfile = L"vs_6_9";
                break;
            case ShaderType::Fragment:
                targetProfile = L"ps_6_9";
                break;
            case ShaderType::Compute:
                targetProfile = L"cs_6_9";
                break;
            case ShaderType::Geometry:
                targetProfile = L"gs_6_9";
                break;
            case ShaderType::TessControl:
                targetProfile = L"hs_6_9";
                break;
            case ShaderType::TessEvaluation:
                targetProfile = L"ds_6_9";
                break;
            default:
                throw std::runtime_error("Unknown shader type");
        }

        //convert entry point to wide string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        auto entryPoint_w= conv.from_bytes(entryPoint);
        auto shaderFileName_w= conv.from_bytes(fileName);

        // Configure the compiler arguments for compiling the HLSL shader to SPIR-V
        std::vector<LPCWSTR> arguments = {
            // (Optional) name of the shader file to be displayed e.g. in an error message
            shaderFileName_w.c_str(),
            // Shader main entry point
            L"-E", L"main",
            // Shader target profile
            L"-T", targetProfile,
            // Compile to SPIRV

            L"-spirv",
            L"-fspv-target-env=vulkan1.3",
            L"-fspv-preserve-bindings",
            L"-fspv-preserve-interface",
        };

        if (debug)
        {
            arguments.push_back(L"-O0");
            arguments.push_back(L"-fspv-reflect");
        }
        else
        {
            arguments.push_back(L"-O3");
        }

        // Compile shader
        DxcBuffer buffer{};
        buffer.Encoding = DXC_CP_ACP;
        buffer.Ptr = sourceBlob->GetBufferPointer();
        buffer.Size = sourceBlob->GetBufferSize();

        ComPtrLite<IDxcResult> result{nullptr};
        hres = compiler->Compile(
            &buffer,
            arguments.data(),
            (uint32_t) arguments.size(),
            nullptr,
            IID_PPV_ARGS(&result));

        if (SUCCEEDED(hres))
        {
            result->GetStatus(&hres);
        }

        // Output error if compilation failed
        if (FAILED(hres) && (result))
        {
            ComPtrLite < IDxcBlobEncoding > errorBlob;
            hres = result->GetErrorBuffer(&errorBlob);
            if (SUCCEEDED(hres) && errorBlob)
            {
                throw std::runtime_error(static_cast<const char *>(errorBlob->GetBufferPointer()));
            }
        }

        // Get compilation result
        ComPtrLite < IDxcBlob > code;
        result->GetResult(&code);


        std::vector<uint32_t> spirvCode;
        spirvCode.resize(code->GetBufferSize() / sizeof(uint32_t));
        memcpy(spirvCode.data(), code->GetBufferPointer(), code->GetBufferSize());
        return spirvCode;
    }
}
