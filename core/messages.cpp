#include <iostream>
#include <mutex>

#include "EngineBase.h"
#include "SystemBeckend.hpp"

namespace vkbase
{
    extern Event<std::string_view, MessageType> onMessage;
    std::recursive_mutex streamMutex;

    void info_internal(const char* str, int len)
    {
        sys::info(str,len);
        onMessage.call(std::string_view(str,len),MessageType::Info);
    }

    void error_internal(const char* str, int len)
    {
        sys::error(str,len);
        onMessage.call(std::string_view(str,len),MessageType::Error);
    }

    void warning_internal(const char* str, int len)
    {
        sys::warning(str,len);
        onMessage.call(std::string_view(str,len),MessageType::Warning);
    }

    void info(const std::string &str)
    {
        info_internal(str.c_str(),static_cast<int>(str.size()));
        info_internal("\n",1);
    }
    void error(const std::string &str)
    {
        error_internal(str.c_str(), static_cast<int>(str.size()));
        error_internal("\n",1);
    }

    void warning(const std::string &str)
    {
        warning_internal(str.c_str(), static_cast<int>(str.size()));
        warning_internal("\n",1);
    }


    class BufferedStream: public std::streambuf
    {
    public:
        using Sink = void(*)(const char*, int);

        explicit BufferedStream(Sink sink):sink(sink)
        {
            buffer.reserve(256);
        }

    protected:
        int overflow(int c) override
        {
            if (traits_type::eq_int_type(c, traits_type::eof()))
                return sync() == 0 ? traits_type::not_eof(c) : traits_type::eof();

            buffer.push_back(traits_type::to_char_type(c));
            if (traits_type::to_char_type(c) == '\n')
                flush();
            return c;
        }

        std::streamsize xsputn(const char* s, std::streamsize count) override
        {
            std::string_view view{s, static_cast<size_t>(count)};
            size_t start = 0;

            while (start < view.size())
            {
                size_t newlinePos = view.find('\n', start);
                if (newlinePos == std::string_view::npos)
                {
                    buffer.append(view.substr(start));
                    break;
                }

                buffer.append(view.substr(start, newlinePos - start + 1));
                flush();
                start = newlinePos + 1;
            }

            return count;
        }

        int sync() override
        {
            flush();
            return 0;
        }

    private:
        void flush()
        {
            if (buffer.empty())
                return;

            std::string message;
            message.swap(buffer);

            std::lock_guard<std::recursive_mutex> lock(streamMutex);
            sink(message.c_str(), static_cast<int>(message.size()));
        }

        Sink sink;
        std::string buffer;
    } infoStream(info_internal), errorStream(error_internal);

    std::streambuf* oldCoutStreamBuf = nullptr;
    std::streambuf* oldCerrStreamBuf = nullptr;

    void redirectOut()
    {
        if (oldCoutStreamBuf == nullptr)
            oldCoutStreamBuf = std::cout.rdbuf(&infoStream);
        if (oldCerrStreamBuf == nullptr)
            oldCerrStreamBuf = std::cerr.rdbuf(&errorStream);
    }

    void restoreOut()
    {
        if (oldCoutStreamBuf != nullptr)
        {
            std::cout.rdbuf(oldCoutStreamBuf);
            oldCoutStreamBuf = nullptr;
        }

        if (oldCerrStreamBuf != nullptr)
        {
            std::cerr.rdbuf(oldCerrStreamBuf);
            oldCerrStreamBuf = nullptr;
        }
    }
}
