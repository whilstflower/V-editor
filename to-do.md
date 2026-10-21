# Human Actions To Finish the Native Mac Replacement

## Before Mac Testing

- [ ] Provide access to an Intel Mac running macOS 15.7.7 for native build and runtime testing.
- [ ] Install Qt Creator, a Qt 6 Widgets `x86_64` kit, CMake 3.20+, and Xcode or the Xcode Command Line Tools.
- [ ] Open `native/CMakeLists.txt` in Qt Creator, select the macOS `x86_64` kit, and set `CMAKE_OSX_ARCHITECTURES` to `x86_64` as described in [native/README-macOS.md](native/README-macOS.md).
- [ ] Connect a VG-88 through a macOS-compatible MIDI interface that supports SysEx, and install any required interface driver.
- [ ] Back up the VG-88 user bank and identify a user patch slot that may be safely overwritten during send tests.
- [ ] Confirm the VG-88 MIDI device ID so it can be entered in the native app.
- [ ] Provide representative `.syx` files for single patches, full banks, presets, and any unusual libraries that must remain compatible.

## Build and Acceptance Checks

- [ ] Build the `VEditorNative` Release target and run both included tests in Qt Creator as described in [native/README-macOS.md](native/README-macOS.md).
- [ ] Launch the app and open both a single-patch file and a full bank; verify patch count, bank/patch numbers, names, and readable errors for unsupported or damaged files.
- [ ] Rename a patch, save to a new file, reopen it, and confirm the new name while the original source file remains unchanged.
- [ ] Confirm that sending a patch to the backed-up test slot updates the intended VG-88 patch and that the patch recalls correctly on the device.
- [ ] Disconnect/reconnect the MIDI interface and verify the output list can be refreshed and reports unavailable devices clearly.
- [ ] Compare the finished native app against the Windows editor's required workflows and record any behavior that differs before accepting feature parity.
- [ ] Confirm the delivered app launches on the target Mac without development tools installed and document any required Qt runtime or driver setup.

## Human Sign-Off

- [ ] Review the remaining feature-parity gaps with the port developer and confirm the required workflows before calling the native app a replacement.
- [ ] Approve the tested build only after file compatibility and hardware send tests pass on the target Intel Mac.
