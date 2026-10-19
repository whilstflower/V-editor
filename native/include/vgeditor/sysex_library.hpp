#pragma once

#include <cstdint>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace vgeditor {

struct Patch {
    std::uint8_t number = 0;
    std::string name;
    std::vector<std::uint8_t> payload;
    bool modified = false;
};

std::vector<std::vector<std::uint8_t>> encodePatchMessages(
    const Patch& patch,
    std::uint8_t deviceId);

class SysexLibrary {
public:
    static SysexLibrary load(const std::filesystem::path& path);

    const std::vector<Patch>& patches() const noexcept;
    const std::vector<std::uint8_t>& originalBytes() const noexcept;
    void renamePatch(std::size_t index, const std::string& name);
    std::vector<std::uint8_t> serializedBytes() const;
    void markSaved();

private:
    struct FrameRange {
        std::size_t offset = 0;
        std::size_t length = 0;
    };

    std::vector<Patch> patches_;
    std::vector<std::array<FrameRange, 6>> frameRanges_;
    std::vector<std::uint8_t> originalBytes_;
};

} // namespace vgeditor