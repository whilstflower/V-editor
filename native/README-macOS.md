# Native macOS build

This is a native Qt Widgets port for Intel Macs. It does not use Wine or MFC.
The current native app opens VG-88 user-patch SysEx libraries, lists their
patches, displays decoded names and raw patch data, and saves an unchanged
library byte-for-byte. Patch names can be edited and saved while all unrelated
SysEx messages are preserved. CoreMIDI output enumeration and selected-patch
SysEx transmission are also implemented.

## Requirements

- An Intel Mac running macOS 12 or later
- Xcode or the Xcode Command Line Tools
- Qt Creator
- Qt 6 Widgets for macOS `x86_64`
- CMake 3.20 or later (available to Qt Creator)

Install Qt Creator and Qt with the official Qt installer. Select a Qt 6
macOS `x86_64` kit with the Widgets component, and make sure the kit uses the
Xcode compiler toolchain.

## Open and configure the project

1. Open Qt Creator and choose **File > Open File or Project**.
2. Select `native/CMakeLists.txt` from this repository.
3. When prompted, select the Qt 6 macOS `x86_64` kit and create a build
   directory, for example `native/build-macos`.
4. In the CMake configuration settings for the project, add
   `CMAKE_OSX_ARCHITECTURES` with the value `x86_64`. The selected Qt kit
   supplies the Qt installation to CMake; no Qt path needs to be entered in a
   terminal.
5. Run CMake configuration from Qt Creator and check that it completes without
   errors.

## Build, test, and run

Choose the `Release` configuration in Qt Creator, then use **Build > Build
Project**. The `VEditorNative` target builds the macOS application bundle.

Run the included tests from Qt Creator's **Tests** view after the project has
configured. If the tests are not shown, enable CTest test discovery in the
project's test settings and reconfigure. Run both `sysex_library_test` and
`sysex_single_patch_test`.

Select `VEditorNative` as the run target and use **Run** to launch the app.
The first build may take longer while Qt Creator configures CMake and compiles
the project.

## Port status

The original MFC application cannot be compiled as a macOS binary. The Qt app
is a new frontend over a portable SysEx library reader. VG-88 parameter editing,
library management, MIDI input/receive workflows, and most legacy dialogs are
not yet ported; the native app must not be treated as a feature-complete
replacement for the Windows release. The existing Windows sources remain
unchanged.