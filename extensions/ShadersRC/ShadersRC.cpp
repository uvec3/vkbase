#include "ShadersRC.hpp"


#include <execution>
#include <locale>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "slang/slang/external/spirv-tools/include/spirv-tools/libspirv.hpp"
#include "spirv-tools/libspirv.h"

namespace vkbase::ShadersRC
{
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
        // spvtools::SpirvTools spirv_tools(SPV_ENV_UNIVERSAL_1_6);
        // spirv_tools.SetMessageConsumer([](spv_message_level_t level, const char *, const spv_position_t &position,
        //                                   const char *message) {
        //     std::string level_str;
        //     switch (level) {
        //         case SPV_MSG_FATAL:
        //             level_str = "FATAL";
        //             break;
        //         case SPV_MSG_INTERNAL_ERROR:
        //             level_str = "INTERNAL ERROR";
        //             break;
        //         case SPV_MSG_ERROR:
        //             level_str = "ERROR";
        //             break;
        //         case SPV_MSG_WARNING:
        //             level_str = "WARNING";
        //             break;
        //         case SPV_MSG_INFO:
        //             level_str = "INFO";
        //             break;
        //         case SPV_MSG_DEBUG:
        //             level_str = "DEBUG";
        //             break;
        //     }
        //     throw std::runtime_error("SPIR-V Optimization " + level_str + " at " +
        //                              std::to_string(position.index) + ": " + message);
        // });
        //
        // std::vector<uint32_t> optimized_spirv;
        // spvtools::OptimizerOptions options;
        // options.set_run_validator(true);
        //
        // bool success = spirv_tools.(
        //     spirv_code.data(), spirv_code.size(),
        //     spvtools::OptimizerOptions(), &optimized_spirv);
        //
        // if (!success) {
        //     throw std::runtime_error("SPIR-V optimization failed.");
        // }
        //
        // return optimized_spirv;
        return {};
    }
}
