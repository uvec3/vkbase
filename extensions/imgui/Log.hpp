#pragma once

#include <vector>
#include <string>
#include "imgui.hpp"
#include <mutex>

#include "../../core/glm/glm/glm.hpp"

namespace vkbase::imgui
{
    class Log
    {
    public:

        struct TextColor
        {
            uint64_t posStart;
            glm::vec4 color;
        };

        explicit Log(const char *name = "Log");
        void clear();

        void draw(bool* show);
        void draw_as_child();
        void add(const std::string_view &str, glm::vec4 color=glm::vec4(-1));
        std::string getName() const;
        std::string getContent();
        void setName(const std::string & string);

    private:
        std::string name = "Log";
        std::vector<uint64_t> lines;
        std::vector<TextColor> colors;
        std::string content;
        std::mutex m;

        void draw_internal();
    };


    void registerEngineMessagesLog();
    void unregisterEngineMessagesLog();
    void drawEngineMessagesLog(bool* show = nullptr);
}