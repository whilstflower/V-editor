cmake_minimum_required(VERSION 3.20)
project(VEditorNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vgeditor_sysex src/sysex_library.cpp)
target_include_directories(vgeditor_sysex PUBLIC include)

add_executable(sysex_library_test tests/sysex_library_test.cpp)
target_link_libraries(sysex_library_test PRIVATE vgeditor_sysex)

enable_testing()
add_test(NAME sysex_library_test
    COMMAND sysex_library_test "${CMAKE_CURRENT_SOURCE_DIR}/../Patch/default.syx")
add_test(NAME sysex_single_patch_test
    COMMAND sysex_library_test "${CMAKE_CURRENT_SOURCE_DIR}/../Patch/nilon2ID1.syx")

if(APPLE)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    add_executable(VEditorNative MACOSX_BUNDLE
        src/core_midi.cpp
        src/main.cpp
        src/main_window.cpp
        include/vgeditor/core_midi.hpp
        include/vgeditor/main_window.hpp
    )
    find_library(COREMIDI_FRAMEWORK CoreMIDI REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(VEditorNative PRIVATE vgeditor_sysex Qt6::Widgets
        ${COREMIDI_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
    target_include_directories(VEditorNative PRIVATE include)
    set_target_properties(VEditorNative PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "V-Editor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.veditor.native"
    )
endif()