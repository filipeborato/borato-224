cmake_minimum_required(VERSION 3.22)

project(Borato224 VERSION 0.0.6 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BORATO224_FETCH_JUCE "Download JUCE if it is not found locally" OFF)
option(BORATO224_COPY_PLUGIN_AFTER_BUILD "Copy the built plugin to the system plugin folder after build" OFF)
option(BORATO224_BUILD_CLAP "Build the CLAP plugin through clap-juce-extensions" ON)
option(BORATO224_CLAP_FROM_JUCER "Build only a CLAP wrapper from the Projucer-generated Shared Code library" OFF)
option(BORATO224_BUILD_TESTS "Build Borato 224 state-management tests" OFF)
set(BORATO224_JUCE_SOURCE_DIR "" CACHE PATH "Path to a JUCE source checkout")
set(BORATO224_JUCER_CONFIGURATION "Release" CACHE STRING "Projucer Shared Code configuration to link when BORATO224_CLAP_FROM_JUCER is enabled")
set(BORATO224_JUCER_SHARED_CODE_LIB "" CACHE FILEPATH "Path to the Projucer-generated Shared Code library")

if(NOT BORATO224_JUCE_SOURCE_DIR AND EXISTS "C:/JUCE/CMakeLists.txt")
    set(BORATO224_JUCE_SOURCE_DIR "C:/JUCE")
endif()

if(BORATO224_JUCE_SOURCE_DIR AND EXISTS "${BORATO224_JUCE_SOURCE_DIR}/CMakeLists.txt")
    set(JUCE_BUILD_HELPER_TOOLS OFF CACHE BOOL "Bootstrap JUCE helper tools at configure time" FORCE)
    add_subdirectory("${BORATO224_JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
else()
    find_package(JUCE CONFIG QUIET)
    if(NOT JUCE_FOUND)
        if(NOT BORATO224_FETCH_JUCE)
            message(FATAL_ERROR "JUCE not found. Set BORATO224_JUCE_SOURCE_DIR/JUCE_DIR/CMAKE_PREFIX_PATH or enable BORATO224_FETCH_JUCE.")
        endif()

        include(FetchContent)
        FetchContent_Declare(
            juce
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.12
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(juce)
    endif()
endif()

if(BORATO224_BUILD_CLAP OR BORATO224_CLAP_FROM_JUCER)
    set(BORATO224_CLAP_EXTENSIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions")
    if(EXISTS "${BORATO224_CLAP_EXTENSIONS_DIR}/CMakeLists.txt")
        add_subdirectory("${BORATO224_CLAP_EXTENSIONS_DIR}" "${CMAKE_BINARY_DIR}/clap-juce-extensions" EXCLUDE_FROM_ALL)
    else()
        message(FATAL_ERROR "clap-juce-extensions not found. Run: git submodule update --init --recursive")
    endif()
endif()

if(BORATO224_CLAP_FROM_JUCER)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode/JuceHeader.h")
        message(FATAL_ERROR "JuceLibraryCode was not found. Open Borato224.jucer in Projucer, check the JUCE module paths, and save the project first.")
    endif()

    if(NOT BORATO224_JUCER_SHARED_CODE_LIB)
        if(WIN32)
            set(BORATO224_JUCER_SHARED_CODE_LIB
                "${CMAKE_CURRENT_SOURCE_DIR}/Builds/VisualStudio2026/x64/${BORATO224_JUCER_CONFIGURATION}/Shared Code/Borato224.lib")
        elseif(APPLE)
            set(BORATO224_JUCER_SHARED_CODE_LIB
                "${CMAKE_CURRENT_SOURCE_DIR}/Builds/MacOSX/build/${BORATO224_JUCER_CONFIGURATION}/libBorato224.a")
        else()
            set(BORATO224_JUCER_SHARED_CODE_LIB
                "${CMAKE_CURRENT_SOURCE_DIR}/Builds/LinuxMakefile/build/libBorato224.a")
        endif()
    endif()

    if(NOT EXISTS "${BORATO224_JUCER_SHARED_CODE_LIB}")
        message(FATAL_ERROR "Projucer Shared Code library not found: ${BORATO224_JUCER_SHARED_CODE_LIB}. Build the Projucer Shared Code target first, or set BORATO224_JUCER_SHARED_CODE_LIB explicitly.")
    endif()

    clap_juce_extensions_plugin_jucer(
        TARGET Borato224Projucer
        TARGET_PATH "${BORATO224_JUCER_SHARED_CODE_LIB}"
        PLUGIN_BINARY_NAME "Borato 224"
        PLUGIN_VERSION "${PROJECT_VERSION}"
        CLAP_ID "com.boratoaudio.borato224"
        CLAP_FEATURES audio-effect reverb stereo)

    target_include_directories(Borato224Projucer_CLAP
        PUBLIC
            "${BORATO224_JUCE_SOURCE_DIR}/modules"
            "${CMAKE_CURRENT_SOURCE_DIR}/JuceLibraryCode")

    target_compile_definitions(Borato224Projucer_CLAP
        PRIVATE
            JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JucePlugin_Name="Borato 224"
            JucePlugin_Manufacturer="Borato Company"
            JucePlugin_ManufacturerWebsite="https://boratoaudio.com"
            JucePlugin_VersionString="${PROJECT_VERSION}"
            JucePlugin_Desc=""
            JucePlugin_IsSynth=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ManufacturerCode=Bora
            JucePlugin_PluginCode="B224"
            JucePlugin_VSTUniqueID="B224"
            JucePlugin_EditorRequiresKeyboardFocus=0)

    if(MSVC)
        target_compile_options(Borato224Projucer_CLAP PRIVATE /W4 /permissive- /wd4244 /wd4267)
    endif()

    return()
endif()

# JUCE 8's Windows resource helper may be emitted as the literal command
# "juce::juceaide" by the VS 2026 generator in this environment. Redefine the
# helper locally so the generated project uses the resolved executable path.
if(WIN32)
    function(_juce_add_resources_rc source_target dest_target)
        if(NOT TARGET ${source_target}_rc_lib)
            get_target_property(juce_library_code ${source_target} JUCE_GENERATED_SOURCES_DIRECTORY)
            get_target_property(input_info_file ${source_target} JUCE_INFO_FILE)
            get_target_property(generated_icon ${source_target} JUCE_ICON_FILE)

            set(dependency)
            if(generated_icon)
                set(dependency DEPENDS "${generated_icon}")
            endif()

            set(resource_rc_file "${juce_library_code}/${source_target}_resources.rc")

            add_custom_command(OUTPUT "${resource_rc_file}"
                COMMAND "$<TARGET_FILE:juce::juceaide>" rcfile "${input_info_file}" "${resource_rc_file}"
                ${dependency}
                VERBATIM)

            add_library(${source_target}_rc_lib OBJECT "${resource_rc_file}")
        endif()

        target_link_libraries(${dest_target} PRIVATE ${source_target}_rc_lib)
    endfunction()
endif()

juce_add_plugin(Borato224
    COMPANY_NAME "Borato Company"
    BUNDLE_ID com.boratoaudio.borato224
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${BORATO224_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Bora
    PLUGIN_CODE B224
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Borato 224"
)

juce_generate_juce_header(Borato224)

juce_add_binary_data(Borato224Assets
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        assets/lexicon_224.svg
        assets/faders_224.svg
        assets/leds_224.svg
        assets/push_224.svg
)

target_sources(Borato224
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginParameters.cpp
        Source/PluginParameters.h
        Source/dsp/ReverbEngine.cpp
        Source/dsp/ReverbEngine.h
        Source/ui/Borato224LookAndFeel.cpp
        Source/ui/Borato224LookAndFeel.h
        Source/ui/DisplayComponent.cpp
        Source/ui/DisplayComponent.h
        Source/ui/ProgramButtonsComponent.cpp
        Source/ui/ProgramButtonsComponent.h
        Source/ui/SystemButtonsComponent.cpp
        Source/ui/SystemButtonsComponent.h
        Source/ui/FaderBayComponent.cpp
        Source/ui/FaderBayComponent.h
        Source/ui/PcbComponent.cpp
        Source/ui/PcbComponent.h
)

target_link_libraries(Borato224
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        Borato224Assets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(Borato224
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    target_compile_options(Borato224 PRIVATE /W4 /permissive- /wd4244 /wd4267)
else()
    target_compile_options(Borato224 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BORATO224_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET Borato224
        CLAP_ID "com.boratoaudio.borato224"
        CLAP_FEATURES audio-effect reverb stereo)
endif()

if(BORATO224_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(Borato224StateTests PRODUCT_NAME "Borato 224 State Tests")
    target_sources(Borato224StateTests PRIVATE tests/StateTests.cpp)
    target_include_directories(Borato224StateTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_BINARY_DIR}/Borato224_artefacts/JuceLibraryCode"
        "${CMAKE_BINARY_DIR}/juce_binarydata_Borato224Assets/JuceLibraryCode")
    target_link_libraries(Borato224StateTests PRIVATE Borato224 juce::juce_audio_utils)
    add_test(NAME Borato224StateTests COMMAND Borato224StateTests)
endif()
