// A fresh directory per test (specs/testing.md §5): under the build tree, removed when the test
// passes and kept when it fails so the files can be inspected.
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ez::test {

class TempDir {
public:
    explicit TempDir(std::string_view name);
    ~TempDir();
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] std::filesystem::path operator/(std::string_view child) const { return path_ / child; }

    // Writes a whole file; returns its path.
    std::filesystem::path write(std::string_view name, std::string_view content) const;
    // Reads a whole file, or returns "" when it does not exist.
    [[nodiscard]] std::string read(std::string_view name) const;

private:
    std::filesystem::path path_;
};

}  // namespace ez::test
