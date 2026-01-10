#include "Log.hpp"
#include "imgui.hpp"
#include "imgui/imgui_internal.h"
#include "../../core/EngineBase.h"

namespace vkbase::imgui
{
    Log::Log(const char *name)
    {
        this->name = name;
    }

    void Log::clear()
    {
        mutex.lock();
        lines.clear();
        content.clear();
        mutex.unlock();
    }

    void Log::draw()
    {
        if (!show)
            return;

        if (ImGui::Begin(name.c_str()), &show)
        {
          draw_internal();
        }
        ImGui::End();
    }

    void Log::draw_as_child()
    {
        if (!show)
            return;

        if (ImGui::BeginChild(name.c_str(),ImVec2(0,0),0,ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::Text("Log");
            draw_internal();
        }
        ImGui::EndChild();
    }

    void Log::add(const std::string_view &str, glm::vec4 color)
    {
        mutex.lock();
        if(colors.empty() || colors.back().color!=color)
            colors.push_back({.posStart= content.size(),.color= color});
        if(lines.empty())
            lines.push_back(0);
        uint64_t start = -1;
        uint64_t strBegin = content.size();
        while (start = str.find('\n', start + 1), start != std::string::npos )
        {
            lines.push_back(strBegin + start + 1);
        }
        content+=str;
        mutex.unlock();
    }

    std::string Log::getName() const
    {
        return name;
    }

    void Log::setName(const std::string &string)
    {
        name = string;
    }


    static Log uiLog;
    static int messageEventId=-1;
    void message(std::string_view msg, vkbase::MessageType type)
    {
        if(type==MessageType::Info)
            uiLog.add(msg);
        else  if(type==MessageType::Error)
            uiLog.add(msg,glm::vec4(1,0,0,1));
        else if(type==MessageType::Warning)
            uiLog.add(msg, glm::vec4(1,1,0,1));
    }

    void Log::draw_internal()
    {
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear"))
                clear();
            if (ImGui::Selectable("Copy"))
                ImGui::SetClipboardText(content.c_str());
            ImGui::EndPopup();
        }

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(lines.size()));

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                //ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);

                char *start = content.data() + lines[i];
                char *end;
                if (i + 1 == lines.size())
                    end = content.data() + content.size();
                else
                    end = content.data() + lines[i + 1];


                ImGui::TextUnformatted(start, end);
            }
        }
        clipper.End();


        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }

    void registerEngineMessagesLog()
    {
        messageEventId= addMessagesListener(message);
    }

    void unregisterEngineMessagesLog()
    {
        removeMessagesListener(messageEventId);
    }

    void drawEngineMessagesLog()
    {
        uiLog.draw();
    }
}