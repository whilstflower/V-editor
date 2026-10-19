# Native macOS build

This is a native Qt Widgets port for Intel Macs. It does not use Wine or MFC.
The current native app opens VG-88 user-patch SysEx libraries, lists their
patches, displays decoded names and raw patch data, and saves an unchanged
library byte-for-byte. Patch names can be edited and saved while all unrelated
SysEx messages are preserved. CoreMIDI output enumeration and selected-patch
SysEx transmission are also implemented.

## Requirements

- An Intel Mac running macOS 12 or later
- Xcode Command Line Tools
- CMake 3.20 or later
- Qt 6 Widgets built for macOS `x86_64`

Install Qt with the official Qt installer and select the macOS `x86_64` kit.
Then configure and build from the repository root:

```sh
cmake -S native -B build-macos \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.3/macos"
cmake --build build-macos --config Release
ctest --test-dir build-macos --output-on-failure
open build-macos/VEditorNative.app
```

Adjust `CMAKE_PREFIX_PATH` to the Qt version and install location on the Mac.

## Port status

The original MFC application cannot be compiled as a macOS binary. The Qt app
is a new frontend over a portable SysEx library reader. VG-88 parameter editing,
library management, MIDI input/receive workflows, and most legacy dialogs are
not yet ported; the native app must not be treated as a feature-complete
replacement for the Windows release. The existing Windows sources remain
unchanged.