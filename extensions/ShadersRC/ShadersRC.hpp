#pragma once
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include "ShaderType.hpp"
#include "SPIRV-Reflect/spirv_reflect.h"
#include <ranges>

namespace vkbase::ShadersRC
{
    struct SPIRVReflection
    {
        SpvReflectShaderModule module;
        std::vector<SpvReflectDescriptorBinding *> descriptorBindings;
        std::vector<SpvReflectDescriptorSet *> descriptorSets;
        std::vector<SpvReflectUserType> userTypes;


        SPIRVReflection(const std::vector<uint32_t> &spirv_code)
        {
            SpvReflectResult result = spvReflectCreateShaderModule(
                spirv_code.size() * sizeof(uint32_t),
                spirv_code.data(),
                &module);
            if (result != SPV_REFLECT_RESULT_SUCCESS)
            {
                throw std::runtime_error("Failed to create SPIR-V reflection module.");
            }

            uint32_t var_count = 0;
            result = spvReflectEnumerateDescriptorBindings(&module, &var_count, nullptr);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
            descriptorBindings.resize(var_count);
            result = spvReflectEnumerateDescriptorBindings(&module, &var_count, descriptorBindings.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            result = spvReflectEnumerateDescriptorSets(&module, &var_count, nullptr);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
            descriptorSets.resize(var_count);
            result = spvReflectEnumerateDescriptorSets(&module, &var_count, descriptorSets.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
        }

        ~SPIRVReflection()
        {
            spvReflectDestroyShaderModule(&module);
        }
    };


    std::string spirv_to_text(const std::vector<uint32_t> &spirv_code, bool for_print=false);
    std::vector<uint32_t> text_to_spirv(const std::string &text);
    std::vector<uint32_t> optimize_spirv(const std::vector<uint32_t> &spirv_code);

    class SPIRVEditor
    {
        std::vector<uint32_t> spirv_code;
        std::string text;
        std::vector<std::vector<std::string>> word_lines;

    public:

        constexpr static std::vector<std::string> split(const std::string &s, char delimiter=' ')
        {
            std::vector<std::string> result;
            size_t word_start = 0;
            size_t word_end = 0;
            while ((word_end = s.find(delimiter, word_start))!=std::string::npos)
            {
                result.push_back(s.substr(word_start,word_end-word_start));
                word_start=word_end+1;
            }
            result.push_back(s.substr(word_start));
            return result;
        }

        explicit SPIRVEditor(const std::vector<uint32_t> &spirv_code)
        {
            this->spirv_code = spirv_code;
            text= spirv_to_text(spirv_code);
            size_t pos = 0;
            size_t line_start = 0;
            while ((pos = text.find('\n', line_start)) != std::string::npos)
            {
                std::string line = text.substr(line_start, pos - line_start);
                word_lines.push_back(split(line,' '));
                line_start = pos + 1;
            }
        }

        //returns view of key value lines where key is line number matching the pattern
        std::vector<int> search_pattern(const std::vector<std::string>& pattern) const
        {
            std::vector<int> result;
            for (size_t line_index=0;line_index<word_lines.size();++line_index)
            {
                const auto& line_words=word_lines[line_index];
                if (line_words.size()<pattern.size())
                    continue;

                bool match=true;
                for (size_t i=0;i<pattern.size();++i)
                {
                    if (pattern[i]!="*" && line_words[i]!=pattern[i])
                    {
                        match=false;
                        break;
                    }
                }

                if (match)
                    result.push_back(static_cast<int>(line_index));
            }

            return result;
        }


        int first_match(const std::vector<std::string>& pattern) const
        {
            for (size_t line_index=0;line_index<word_lines.size();++line_index)
            {
                const auto& line_words=word_lines[line_index];
                if (line_words.size()<pattern.size())
                    continue;

                bool match=true;
                for (size_t i=0;i<pattern.size();++i)
                {
                    if (pattern[i]!="*" && line_words[i]!=pattern[i])
                    {
                        match=false;
                        break;
                    }
                }
                if (match)
                    return static_cast<int>(line_index);
            }
            return -1;
        }

        std::vector<std::string>& search_line(const std::vector<std::string>& pattern)
        {
            auto line_index=first_match(pattern);
            if (line_index<0)
                throw std::runtime_error("Pattern not found in SPIR-V code.");
            return word_lines[line_index];
        }

        std::string variable_name_by_binding(uint32_t set, uint32_t binding)
        {
            for (auto line_index:search_pattern(split("OpDecorate * DescriptorSet *")))
            {
                const auto& line=word_lines[line_index];
                if (line[3]==std::to_string(set))
                {
                    auto var_name=line[1];
                    auto binding_line_index=search_pattern({"OpDecorate",var_name,"Binding","*"});
                    if (!binding_line_index.empty())
                    {
                        const auto& binding_line=word_lines[binding_line_index[0]];
                        if (binding_line[3]==std::to_string(binding))
                        {
                            return var_name;
                        }
                    }
                }
            }
            return "";
        }

        std::string pointer_by_variable_name(const std::string& variable)
        {
            auto& line=search_line({variable, "=", "OpVariable", "*", "*"});
            return line[3];
        }



        void set_array_stride_for_storage_buffer(const std::string& variable, uint32_t new_stride)
        {
            std::string suffix="st"+std::to_string(new_stride);
            auto variable_line_index=first_match({variable,"="});
            auto& variable_line=word_lines[variable_line_index];
            auto pointer=pointer_by_variable_name(variable);
            auto type_pointer_line=search_line({pointer,"="});
            auto type_id=type_pointer_line[4];
            auto type_line=search_line({type_id,"="});
            std::vector<std::string> struct_type_line;
            std::optional<std::vector<std::string>> array_line=std::nullopt;
            if (type_line[2]=="OpTypeStruct")
            {
                struct_type_line=type_line;
            }
            else if (type_line[2]=="OpTypeArray")
            {
                array_line=type_line;
                auto struct_type_id=type_line[3];
                struct_type_line=search_line({struct_type_id,"="});
            }

            auto runtime_array_line=search_line({struct_type_line[3],"=","OpTypeRuntimeArray","*"});

            //rename pointer
            type_pointer_line[0]=type_pointer_line[0]+suffix;
            variable_line[3]=type_pointer_line[0];
            //rename type
            type_pointer_line[4]=type_pointer_line[4]+suffix;
            if (array_line.has_value())
            {
                array_line.value()[0]=array_line.value()[0]+suffix;
                array_line.value()[3]=array_line.value()[3]+suffix;
            }
            struct_type_line[0]=struct_type_line[0]+suffix;
            struct_type_line[3]=struct_type_line[3]+suffix;
            //rename runtime array
            runtime_array_line[0]=runtime_array_line[0]+suffix;


            //insert types
            word_lines.insert(word_lines.begin()+variable_line_index,type_pointer_line);
            if (array_line.has_value())
                word_lines.insert(word_lines.begin()+variable_line_index,array_line.value());
            word_lines.insert(word_lines.begin()+variable_line_index,struct_type_line);
            word_lines.insert(word_lines.begin()+variable_line_index,runtime_array_line);


            //insert decorations
            int decorations_line_index=first_match({"OpDecorate"});
            word_lines.insert(word_lines.begin()+decorations_line_index,
                              {"OpDecorate",runtime_array_line[0],"ArrayStride",std::to_string(new_stride)});
            word_lines.insert(word_lines.begin()+decorations_line_index,
                              {"OpDecorate",struct_type_line[0],"Block"});
            word_lines.insert(word_lines.begin()+decorations_line_index,
                   {"OpMemberDecorate", struct_type_line[0],"0","Offset", "0"});
        }

        void set_storage_buffer_offset(const std::string& variable, uint32_t offset)
        {
            std::string suffix="off"+std::to_string(offset);
            auto variable_line_index=first_match({variable,"="});
            auto& variable_line=word_lines[variable_line_index];
            auto pointer=pointer_by_variable_name(variable);
            auto type_pointer_line=search_line({pointer,"="});
            auto type_id=type_pointer_line[4];
            auto type_line=search_line({type_id,"="});
            std::vector<std::string> struct_type_line;
            std::optional<std::vector<std::string>> array_line=std::nullopt;
            if (type_line[2]=="OpTypeStruct")
            {
                struct_type_line=type_line;
            }
            else if (type_line[2]=="OpTypeArray")
            {
                array_line=type_line;
                auto struct_type_id=type_line[3];
                struct_type_line=search_line({struct_type_id,"="});
            }

            // auto runtime_array_line=search_line({struct_type_line[3],"=","OpTypeRuntimeArray","*"});

            //rename pointer
            type_pointer_line[0]=type_pointer_line[0]+suffix;
            variable_line[3]=type_pointer_line[0];
            //rename type
            type_pointer_line[4]=type_pointer_line[4]+suffix;
            if (array_line.has_value())
            {
                array_line.value()[0]=array_line.value()[0]+suffix;
                array_line.value()[3]=array_line.value()[3]+suffix;
            }
            struct_type_line[0]=struct_type_line[0]+suffix;
            //struct_type_line[3]=struct_type_line[3]+suffix;
            //rename runtime array
            // runtime_array_line[0]=runtime_array_line[0]+suffix;


            //insert types
            word_lines.insert(word_lines.begin()+variable_line_index,type_pointer_line);
            if (array_line.has_value())
                word_lines.insert(word_lines.begin()+variable_line_index,array_line.value());
            word_lines.insert(word_lines.begin()+variable_line_index,struct_type_line);
            // word_lines.insert(word_lines.begin()+variable_line_index,runtime_array_line);


            //insert decorations
            int decorations_line_index=first_match({"OpDecorate"});
            // word_lines.insert(word_lines.begin()+decorations_line_index,
            //                   {"OpDecorate",runtime_array_line[0],"ArrayStride",std::to_string(new_stride)});
            word_lines.insert(word_lines.begin()+decorations_line_index,
                              {"OpDecorate",struct_type_line[0],"Block"});
            word_lines.insert(word_lines.begin()+decorations_line_index,
                   {"OpMemberDecorate", struct_type_line[0],"0","Offset", std::to_string(offset)});
        }


        void set_group_size(uint32_t x, uint32_t y, uint32_t z)
        {
            auto& line=search_line({"OpExecutionMode","*","LocalSize","*","*","*"});
            line[3]=std::to_string(x);
            line[4]=std::to_string(y);
            line[5]=std::to_string(z);
        }


        std::vector<uint32_t> get_spirv_code()
        {
            text.clear();
            for (const auto& line_words:word_lines)
            {
                for (size_t i=0;i<line_words.size();++i)
                {
                    text+=line_words[i];
                    if (i!=line_words.size()-1)
                        text+=' ';
                }
                text+='\n';
            }
            spirv_code= vkbase::ShadersRC::text_to_spirv(text);
            return spirv_code;
        }
    };
}
