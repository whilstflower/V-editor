#include "vgeditor/sysex_library.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace vgeditor {
namespace {

constexpr std::uint8_t kRolandManufacturer = 0x41;
constexpr std::uint8_t kDataSet1Command = 0x12;
constexpr std::uint8_t kUserPatchAddress = 0x0c;
constexpr std::size_t kPatchPayloadSize = 704;
constexpr std::size_t kPatchNameOffset = 664;
constexpr std::size_t kBlockPayloadSize = 128;

struct Frame {
    std::uint8_t patchNumber;
    std::uint8_t blockNumber;
    std::size_t offset;
    std::size_t length;
    std::vector<std::uint8_t> payload;
};

std::string decodeName(const std::vector<std::uint8_t>& payload)
{
    std::string name;
    name.reserve(8);

    for (std::size_t i = 0; i < 8; ++i) {
        const auto high = payload[kPatchNameOffset + i * 2];
        const auto low = payload[kPatchNameOffset + i * 2 + 1];
        const auto character = static_cast<unsigned int>(high) * 16u + low;
        name.push_back(character >= 32 && character <= 126
            ? static_cast<char>(character)
            : ' ');
    }

    while (!name.empty() && name.back() == ' ') {
        name.pop_back();
    }

    return name;
}

std::vector<Frame> readPatchFrames(const std::vector<std::uint8_t>& bytes)
{
    std::vector<Frame> frames;
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        if (bytes[offset] != 0xf0) {
            throw std::runtime_error("SysEx data contains bytes outside a message");
        }

        std::size_t end = offset + 1;
        while (end < bytes.size() && bytes[end] != 0xf7) {
            if (bytes[end] > 0x7f) {
                throw std::runtime_error("SysEx message contains an invalid data byte");
            }
            ++end;
        }

        if (end == bytes.size() || end - offset < 12) {
            throw std::runtime_error("SysEx message is truncated");
        }

        const auto* message = bytes.data() + offset;
        const auto messageLength = end - offset + 1;
        if (message[1] == kRolandManufacturer && message[5] == kDataSet1Command
            && message[6] == kUserPatchAddress) {
            Frame frame;
            frame.patchNumber = message[7];
            frame.blockNumber = message[8];
            frame.offset = offset;
            frame.length = messageLength;
            frame.payload.assign(message + 10, message + messageLength - 2);
            frames.push_back(std::move(frame));
        }

        offset = end + 1;
    }

    return frames;
}

} // namespace

std::vector<std::vector<std::uint8_t>> encodePatchMessages(
    const Patch& patch,
    std::uint8_t deviceId)
{
    if (deviceId > 0x7f || patch.number > 0x7f
        || patch.payload.size() != kPatchPayloadSize) {
        throw std::invalid_argument("Patch or MIDI device ID is outside the VG-88 format");
    }

    std::vector<std::vector<std::uint8_t>> messages;
    for (std::size_t block = 0, offset = 0; offset < patch.payload.size(); ++block) {
        const auto length = std::min(kBlockPayloadSize, patch.payload.size() - offset);
        std::vector<std::uint8_t> message{
            0xf0, kRolandManufacturer, deviceId, 0x00, 0x27,
            kDataSet1Command, kUserPatchAddress, patch.number,
            static_cast<std::uint8_t>(block), 0x00};
        message.insert(message.end(), patch.payload.begin() + offset,
            patch.payload.begin() + offset + length);

        unsigned int checksum = kUserPatchAddress + patch.number + block;
        for (std::size_t i = offset; i < offset + length; ++i) {
            checksum += patch.payload[i];
        }
        message.push_back(static_cast<std::uint8_t>((128 - checksum % 128) % 128));
        message.push_back(0xf7);
        messages.push_back(std::move(message));
        offset += length;
    }

    return messages;
}

SysexLibrary SysexLibrary::load(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open SysEx library");
    }

    SysexLibrary library;
    library.originalBytes_ = std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (library.originalBytes_.empty()) {
        throw std::runtime_error("SysEx library is empty");
    }

    const auto frames = readPatchFrames(library.originalBytes_);
    std::array<std::vector<std::uint8_t>, 6> blocks;
    std::array<FrameRange, 6> frameRanges{};
    std::array<bool, 6> received{};
    std::uint8_t activePatch = 0;
    bool hasActivePatch = false;

    for (const auto& frame : frames) {
        if (frame.blockNumber >= blocks.size()) {
            continue;
        }

        if (hasActivePatch && frame.patchNumber != activePatch) {
            blocks = {};
            frameRanges = {};
            received = {};
        }
        activePatch = frame.patchNumber;
        hasActivePatch = true;

        blocks[frame.blockNumber] = frame.payload;
        frameRanges[frame.blockNumber] = {frame.offset, frame.length};
        received[frame.blockNumber] = true;

        if (frame.blockNumber != 5
            || !std::all_of(received.begin(), received.end(), [](bool value) { return value; })) {
            continue;
        }

        Patch patch;
        patch.number = activePatch;
        for (const auto& block : blocks) {
            patch.payload.insert(patch.payload.end(), block.begin(), block.end());
        }

        if (patch.payload.size() != kPatchPayloadSize) {
            throw std::runtime_error("VG-88 patch has an unexpected payload length");
        }

        patch.name = decodeName(patch.payload);
        library.patches_.push_back(std::move(patch));
        library.frameRanges_.push_back(frameRanges);
        blocks = {};
        frameRanges = {};
        received = {};
        hasActivePatch = false;
    }

    if (library.patches_.empty()) {
        throw std::runtime_error("No complete VG-88 user patches were found");
    }

    if (hasActivePatch) {
        throw std::runtime_error("SysEx library ends with an incomplete VG-88 patch");
    }

    return library;
}

const std::vector<Patch>& SysexLibrary::patches() const noexcept
{
    return patches_;
}

const std::vector<std::uint8_t>& SysexLibrary::originalBytes() const noexcept
{
    return originalBytes_;
}

void SysexLibrary::renamePatch(std::size_t index, const std::string& name)
{
    if (index >= patches_.size()) {
        throw std::out_of_range("Patch index is outside the library");
    }
    if (name.size() > 8) {
        throw std::invalid_argument("VG-88 patch names are limited to eight characters");
    }
    for (const auto character : name) {
        if (static_cast<unsigned char>(character) < 32
            || static_cast<unsigned char>(character) > 126) {
            throw std::invalid_argument("VG-88 patch names must use printable ASCII characters");
        }
    }

    auto& patch = patches_[index];
    for (std::size_t i = 0; i < 8; ++i) {
        const auto character = i < name.size()
            ? static_cast<unsigned char>(name[i])
            : static_cast<unsigned char>(' ');
        patch.payload[kPatchNameOffset + i * 2] = character / 16;
        patch.payload[kPatchNameOffset + i * 2 + 1] = character % 16;
    }
    patch.name = name;
    patch.modified = true;
}

std::vector<std::uint8_t> SysexLibrary::serializedBytes() const
{
    auto bytes = originalBytes_;
    for (std::size_t index = 0; index < patches_.size(); ++index) {
        const auto& patch = patches_[index];
        if (!patch.modified) {
            continue;
        }

        const auto deviceId = originalBytes_[frameRanges_[index][0].offset + 2];
        const auto messages = encodePatchMessages(patch, deviceId);
        for (std::size_t block = 0; block < messages.size(); ++block) {
            const auto range = frameRanges_[index][block];
            if (range.length != messages[block].size()) {
                throw std::runtime_error("Edited patch frame size does not match the source library");
            }
            std::copy(messages[block].begin(), messages[block].end(), bytes.begin() + range.offset);
        }
    }
    return bytes;
}

void SysexLibrary::markSaved()
{
    originalBytes_ = serializedBytes();
    for (auto& patch : patches_) {
        patch.modified = false;
    }
}

} // namespace vgeditor