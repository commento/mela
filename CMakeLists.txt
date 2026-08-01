cmake_minimum_required(VERSION 3.22)

project(Mela VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

# Pin JUCE so macOS and Raspberry Pi always build against the same sources.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(Mela
    PRODUCT_NAME "Mela"
    COMPANY_NAME "Mela"
)

target_sources(Mela PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/Audio/LoopEngine.cpp
    Source/Audio/EffectsChain.cpp
    Source/UI/WaveformEditor.cpp
    Source/UI/EffectPanel.cpp
)

target_include_directories(Mela PRIVATE Source)
target_compile_features(Mela PRIVATE cxx_std_17)

target_compile_definitions(Mela PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(Mela PRIVATE
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

juce_generate_juce_header(Mela)
