#include "vgeditor/sysex_library.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: sysex_library_test <library.syx>\n";
        return EXIT_FAILURE;
    }

    try {
        auto library = vgeditor::SysexLibrary::load(argv[1]);
        if (library.patches().empty() || library.patches().front().payload.size() != 704) {
            throw std::runtime_error("No complete VG-88 patch payload was decoded");
        }

        if (std::filesystem::path(argv[1]).filename() == "default.syx") {
            if (library.patches().size() != 100 || library.patches().front().name != "Strat"
                || library.originalBytes().size() != 77600) {
                throw std::runtime_error("Bundled default bank did not decode as expected");
            }
        }

        for (const auto& patch : library.patches()) {
            const auto messages = vgeditor::encodePatchMessages(patch, 0);
            if (messages.size() != 6 || messages.front().size() != 140
                || messages.back().size() != 76) {
                throw std::runtime_error("VG-88 patch was not encoded into six expected frames");
            }
            for (const auto& message : messages) {
                unsigned int checksum = 0;
                for (std::size_t i = 6; i < message.size() - 1; ++i) {
                    checksum += message[i];
                }
                if (checksum % 128 != 0) {
                    throw std::runtime_error("Generated SysEx frame has an invalid checksum");
                }
            }
        }

        library.renamePatch(0, "Native");
        const auto renamedBytes = library.serializedBytes();
        if (renamedBytes.size() != library.originalBytes().size()) {
            throw std::runtime_error("Renaming a patch changed the library size");
        }
        if (std::filesystem::path(argv[1]).filename() == "default.syx") {
            const auto suffixChanged = std::mismatch(renamedBytes.begin() + 776,
                renamedBytes.end(), library.originalBytes().begin() + 776);
            if (suffixChanged.first != renamedBytes.end()) {
                throw std::runtime_error("Renaming a patch changed the wrong SysEx data");
            }
        }

        const auto outputPath = std::filesystem::temp_directory_path()
            / (std::string("veditor-native-renamed-test-")
                + std::filesystem::path(argv[1]).stem().string() + ".syx");
        {
            std::ofstream output(outputPath, std::ios::binary);
            output.write(reinterpret_cast<const char*>(renamedBytes.data()),
                static_cast<std::streamsize>(renamedBytes.size()));
            if (!output) {
                throw std::runtime_error("Unable to write renamed library test data");
            }
        }
        const auto reopened = vgeditor::SysexLibrary::load(outputPath);
        std::error_code removeError;
        std::filesystem::remove(outputPath, removeError);
        if (reopened.patches().front().name != "Native") {
            throw std::runtime_error("Renamed patch did not survive save and reload");
        }
        library.markSaved();
        if (library.patches().front().modified) {
            throw std::runtime_error("Saved patch still reports unsaved changes");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}