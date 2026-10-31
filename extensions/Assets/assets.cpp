#include "assets.h"
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <functional>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
#endif

#if VKBASE_EMBED_ASSETS
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(vkbase_assets);
#endif

namespace vkbase::assets
{
    namespace
    {
        [[maybe_unused]] std::filesystem::path get_executable_directory()
        {
#if defined(_WIN32)
            wchar_t buffer[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            if (len > 0)
            {
                return std::filesystem::path(buffer).parent_path();
            }
#elif defined(__linux__)
            char buffer[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (len != -1)
            {
                buffer[len] = '\0';
                return std::filesystem::path(buffer).parent_path();
            }
#endif
            std::error_code ec;
            return std::filesystem::current_path(ec);
        }

        [[maybe_unused]] std::string normalize_key(std::string key)
        {
            std::replace(key.begin(), key.end(), '\\', '/');
            while (!key.empty() && key.front() == '/')
            {
                key.erase(key.begin());
            }
            return key;
        }

#if !VKBASE_EMBED_ASSETS
        std::filesystem::path resolve_disk_path(const std::string& subpath)
        {
            if (subpath.empty())
                return {};

            std::filesystem::path rel_path(subpath);
            std::error_code ec;

            // 1. Check relative to executable directory (Assets folder first)
            auto exe_dir = get_executable_directory();
            if (!exe_dir.empty())
            {
                auto p = exe_dir / "Assets" / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;

                p = exe_dir / "assets" / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;

                p = exe_dir / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;
            }

            // 2. Check relative to current working directory (Assets folder first)
            auto cur_dir = std::filesystem::current_path(ec);
            if (!cur_dir.empty())
            {
                auto p = cur_dir / "Assets" / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;

                p = cur_dir / "assets" / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;

                p = cur_dir / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;
            }

#if !VKBASE_ASSETS_COPY
            // 3. Baked-in search paths from CMake (only when VKBASE_ASSETS_COPY is false)
            for (const char* const* search_path = internal::SEARCH_PATHS; *search_path != nullptr; ++search_path)
            {
                std::filesystem::path base(*search_path);
                auto p = base / rel_path;
                if (std::filesystem::exists(p, ec))
                    return p;
            }
#endif

            return {};
        }

        std::mutex g_cache_mutex;
        std::unordered_map<std::string, std::string> g_cache;
#endif
    }

    struct directory_iterator::Impl
    {
        virtual ~Impl() = default;
        virtual const directory_entry& current() const = 0;
        virtual void next() = 0;
        virtual bool at_end() const = 0;
    };

#if VKBASE_EMBED_ASSETS
    struct CmrcDirectoryIteratorImpl : public directory_iterator::Impl
    {
        cmrc::directory_iterator m_iter;
        cmrc::directory_iterator m_end;
        std::string m_dir_path;
        directory_entry m_entry;

        CmrcDirectoryIteratorImpl(cmrc::embedded_filesystem fs, const std::string& path)
            : m_dir_path(path)
        {
            try
            {
                if (fs.exists(path) && fs.is_directory(path))
                {
                    m_iter = fs.iterate_directory(path);
                    m_end = m_iter.end();
                    update();
                }
            }
            catch (...)
            {
            }
        }

        void update()
        {
            if (m_iter != m_end)
            {
                auto cmrc_entry = *m_iter;
                std::filesystem::path p = m_dir_path.empty()
                    ? std::filesystem::path(cmrc_entry.filename())
                    : std::filesystem::path(m_dir_path) / cmrc_entry.filename();
                m_entry = directory_entry(p, cmrc_entry.is_file(), cmrc_entry.is_directory());
            }
        }

        const directory_entry& current() const override
        {
            return m_entry;
        }

        void next() override
        {
            if (m_iter != m_end)
            {
                ++m_iter;
                update();
            }
        }

        bool at_end() const override
        {
            return m_iter == m_end;
        }
    };
#else
    struct DiskDirectoryIteratorImpl : public directory_iterator::Impl
    {
        std::filesystem::directory_iterator m_iter;
        std::filesystem::directory_iterator m_end;
        directory_entry m_entry;

        explicit DiskDirectoryIteratorImpl(const std::filesystem::path& dir_path)
        {
            std::error_code ec;
            m_iter = std::filesystem::directory_iterator(dir_path, ec);
            update();
        }

        void update()
        {
            if (m_iter != m_end)
            {
                std::error_code ec;
                const auto& fs_entry = *m_iter;
                m_entry = directory_entry(fs_entry.path(), fs_entry.is_regular_file(ec), fs_entry.is_directory(ec));
            }
        }

        const directory_entry& current() const override
        {
            return m_entry;
        }

        void next() override
        {
            if (m_iter != m_end)
            {
                std::error_code ec;
                m_iter.increment(ec);
                if (ec)
                {
                    m_iter = m_end;
                }
                else
                {
                    update();
                }
            }
        }

        bool at_end() const override
        {
            return m_iter == m_end;
        }
    };
#endif

    directory_iterator::directory_iterator() noexcept
        : m_impl(nullptr)
    {
    }

    directory_iterator::directory_iterator(const std::string& path)
    {
#if VKBASE_EMBED_ASSETS
        auto fs = cmrc::vkbase_assets::get_filesystem();
        std::string norm = normalize_key(path);
        auto impl = std::make_shared<CmrcDirectoryIteratorImpl>(fs, norm);
        if (!impl->at_end())
        {
            m_impl = std::move(impl);
        }
#else
        std::string norm = normalize_key(path);
        auto resolved = resolve_disk_path(norm);
        if (!resolved.empty() && std::filesystem::is_directory(resolved))
        {
            auto impl = std::make_shared<DiskDirectoryIteratorImpl>(resolved);
            if (!impl->at_end())
            {
                m_impl = std::move(impl);
            }
        }
#endif
    }

    const directory_entry& directory_iterator::operator*() const noexcept
    {
        return m_impl->current();
    }

    const directory_entry* directory_iterator::operator->() const noexcept
    {
        return &m_impl->current();
    }

    directory_iterator& directory_iterator::operator++()
    {
        if (m_impl)
        {
            m_impl->next();
            if (m_impl->at_end())
            {
                m_impl = nullptr;
            }
        }
        return *this;
    }

    directory_iterator directory_iterator::operator++(int)
    {
        directory_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool directory_iterator::operator==(const directory_iterator& other) const noexcept
    {
        if (!m_impl && !other.m_impl)
            return true;
        if (!m_impl || !other.m_impl)
            return false;
        return m_impl == other.m_impl;
    }

    bool directory_iterator::operator!=(const directory_iterator& other) const noexcept
    {
        return !(*this == other);
    }

    directory_iterator directory(const std::string& path)
    {
        return directory_iterator(path);
    }

    std::string_view get(const std::string& path)
    {
#if VKBASE_EMBED_ASSETS
        auto fs = cmrc::vkbase_assets::get_filesystem();
        if (!fs.exists(path))
            return "";
        auto file = fs.open(path);
        return {file.cbegin(), file.cend()};
#else
        std::string norm = normalize_key(path);
        {
            std::lock_guard<std::mutex> lock(g_cache_mutex);
            auto it = g_cache.find(norm);
            if (it != g_cache.end())
            {
                return it->second;
            }
        }

        auto resolved = resolve_disk_path(norm);
        if (resolved.empty() || !std::filesystem::is_regular_file(resolved))
        {
            return "";
        }

        std::ifstream file(resolved, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return "";
        }

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string content;
        content.resize(static_cast<size_t>(size));
        if (!file.read(content.data(), size))
        {
            return "";
        }

        std::lock_guard<std::mutex> lock(g_cache_mutex);
        auto [it, _] = g_cache.emplace(std::move(norm), std::move(content));
        return it->second;
#endif
    }

    std::vector<std::pair<std::string, std::string_view>> assetsFrom(const std::string& path)
    {
#if VKBASE_EMBED_ASSETS
        auto fs = cmrc::vkbase_assets::get_filesystem();
        std::vector<std::pair<std::string, std::string_view>> results;

        std::function<void(const std::string&)> visit = [&](const std::string& dir)
        {
            for (const auto& entry : fs.iterate_directory(dir))
            {
                std::string entry_path = dir.empty() ? entry.filename() : dir + "/" + entry.filename();
                if (entry.is_directory())
                {
                    visit(entry_path);
                }
                else
                {
                    auto file = fs.open(entry_path);
                    results.emplace_back(entry_path, std::string_view(file.begin(), file.end()));
                }
            }
        };

        visit(path);
        return results;
#else
        std::string norm = normalize_key(path);
        auto resolved = resolve_disk_path(norm);
        std::vector<std::pair<std::string, std::string_view>> results;
        if (resolved.empty() || !std::filesystem::is_directory(resolved))
        {
            return results;
        }

        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(resolved, ec);
             it != std::filesystem::recursive_directory_iterator();
             it.increment(ec))
        {
            if (ec)
                break;

            if (it->is_regular_file())
            {
                auto rel = std::filesystem::relative(it->path(), resolved, ec);
                if (ec)
                    continue;

                std::string rel_str = rel.generic_string();
                std::string asset_key = norm.empty() ? rel_str : norm + "/" + rel_str;

                auto content = get(asset_key);
                if (content.empty())
                {
                    std::ifstream file(it->path(), std::ios::binary | std::ios::ate);
                    if (file.is_open())
                    {
                        auto size = file.tellg();
                        file.seekg(0, std::ios::beg);
                        std::string buffer;
                        buffer.resize(static_cast<size_t>(size));
                        if (file.read(buffer.data(), size))
                        {
                            std::lock_guard<std::mutex> lock(g_cache_mutex);
                            auto [c_it, _] = g_cache.emplace(asset_key, std::move(buffer));
                            content = c_it->second;
                        }
                    }
                }

                results.emplace_back(std::move(asset_key), content);
            }
        }
        return results;
#endif
    }
}
