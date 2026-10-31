#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <filesystem>
#include <memory>

#include "vkbase_assets_config.h"

namespace vkbase::assets
{
    class directory_entry
    {
    public:
        directory_entry() = default;
        directory_entry(std::filesystem::path path, bool is_regular_file, bool is_directory)
            : m_path(std::move(path)), m_is_regular_file(is_regular_file), m_is_directory(is_directory) {}

        [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }
        [[nodiscard]] operator const std::filesystem::path&() const noexcept { return m_path; }

        [[nodiscard]] bool is_regular_file() const noexcept { return m_is_regular_file; }
        [[nodiscard]] bool is_directory() const noexcept { return m_is_directory; }

    private:
        std::filesystem::path m_path;
        bool m_is_regular_file = false;
        bool m_is_directory = false;
    };

    class directory_iterator
    {
    public:
        struct Impl;

        using iterator_category = std::forward_iterator_tag;
        using value_type = directory_entry;
        using difference_type = std::ptrdiff_t;
        using pointer = const directory_entry*;
        using reference = const directory_entry&;

        directory_iterator() noexcept;
        explicit directory_iterator(const std::string& path);

        const directory_entry& operator*() const noexcept;
        const directory_entry* operator->() const noexcept;
        directory_iterator& operator++();
        directory_iterator operator++(int);

        bool operator==(const directory_iterator& other) const noexcept;
        bool operator!=(const directory_iterator& other) const noexcept;

        directory_iterator begin() const noexcept { return *this; }
        directory_iterator end() const noexcept { return directory_iterator{}; }

    private:
        std::shared_ptr<Impl> m_impl;
    };

    std::vector<std::pair<std::string, std::string_view>> assetsFrom(const std::string& path);
    std::string_view get(const std::string& path);
    directory_iterator directory(const std::string& path);
}
