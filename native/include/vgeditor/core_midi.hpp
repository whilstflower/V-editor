#pragma once

#include "vgeditor/sysex_library.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vgeditor {

struct MidiDestination {
    std::uintptr_t endpoint = 0;
    std::string name;
};

class MidiTransport {
public:
    MidiTransport();
    ~MidiTransport();

    MidiTransport(const MidiTransport&) = delete;
    MidiTransport& operator=(const MidiTransport&) = delete;

    std::vector<MidiDestination> destinations() const;
    void sendPatch(const Patch& patch, std::uint8_t deviceId,
        std::uintptr_t endpoint) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vgeditor