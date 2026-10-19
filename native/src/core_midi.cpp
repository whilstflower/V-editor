#include "vgeditor/core_midi.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>

#include <array>
#include <stdexcept>

namespace vgeditor {

struct MidiTransport::Impl {
    MIDIClientRef client = 0;
    MIDIPortRef outputPort = 0;
    OSStatus status = noErr;

    Impl()
    {
        status = MIDIClientCreate(CFSTR("V-Editor"), nullptr, nullptr, &client);
        if (status == noErr) {
            status = MIDIOutputPortCreate(client, CFSTR("V-Editor Output"), &outputPort);
        }
    }

    ~Impl()
    {
        if (outputPort != 0) {
            MIDIPortDispose(outputPort);
        }
        if (client != 0) {
            MIDIClientDispose(client);
        }
    }
};

MidiTransport::MidiTransport()
    : impl_(std::make_unique<Impl>())
{
}

MidiTransport::~MidiTransport() = default;

std::vector<MidiDestination> MidiTransport::destinations() const
{
    std::vector<MidiDestination> result;
    if (impl_->status != noErr) {
        return result;
    }

    const auto count = MIDIGetNumberOfDestinations();
    for (ItemCount index = 0; index < count; ++index) {
        const auto endpoint = MIDIGetDestination(index);
        if (endpoint == 0) {
            continue;
        }

        CFStringRef displayName = nullptr;
        if (MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName,
                &displayName) != noErr || displayName == nullptr) {
            continue;
        }

        std::array<char, 512> name{};
        const bool converted = CFStringGetCString(displayName, name.data(),
            static_cast<CFIndex>(name.size()), kCFStringEncodingUTF8);
        CFRelease(displayName);
        if (!converted) {
            continue;
        }

        result.push_back({reinterpret_cast<std::uintptr_t>(endpoint), name.data()});
    }

    return result;
}

void MidiTransport::sendPatch(const Patch& patch, std::uint8_t deviceId,
    std::uintptr_t endpoint) const
{
    if (impl_->status != noErr || impl_->outputPort == 0) {
        throw std::runtime_error("CoreMIDI output could not be initialized");
    }
    if (endpoint == 0) {
        throw std::runtime_error("Select a MIDI output device first");
    }

    const auto messages = encodePatchMessages(patch, deviceId);
    const auto destination = reinterpret_cast<MIDIEndpointRef>(endpoint);
    for (const auto& message : messages) {
        alignas(MIDIPacketList) std::array<std::uint8_t, 512> storage{};
        auto* packetList = reinterpret_cast<MIDIPacketList*>(storage.data());
        auto* packet = MIDIPacketListInit(packetList);
        if (MIDIPacketListAdd(packetList, static_cast<ByteCount>(storage.size()), packet,
                0, static_cast<ByteCount>(message.size()), message.data()) == nullptr) {
            throw std::runtime_error("Unable to queue a VG-88 SysEx message");
        }

        const auto result = MIDISend(impl_->outputPort, destination, packetList);
        if (result != noErr) {
            throw std::runtime_error("CoreMIDI failed to send the VG-88 patch");
        }
    }
}

} // namespace vgeditor