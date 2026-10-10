cmake_minimum_required(VERSION 3.25)
project(125A_Metallator VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
add_executable(metallator_core_tests tests/core_tests.cpp)
target_include_directories(metallator_core_tests PRIVATE source)
add_test(NAME metallator_core COMMAND metallator_core_tests)
add_executable(metallator_impact_contract tests/impact_contract_tests.cpp)
target_include_directories(metallator_impact_contract PRIVATE source)
add_test(NAME metallator_impact_contract COMMAND metallator_impact_contract)
add_executable(metallator_stereo_contract tests/stereo_contract_tests.cpp)
target_include_directories(metallator_stereo_contract PRIVATE source)
add_test(NAME metallator_stereo_contract COMMAND metallator_stereo_contract)
add_executable(metallator_diversity_contract tests/diversity_contract_tests.cpp)
target_include_directories(metallator_diversity_contract PRIVATE source)
add_test(NAME metallator_diversity_contract COMMAND metallator_diversity_contract)
add_executable(metallator_seed_matrix tests/seed_matrix_tests.cpp)
target_include_directories(metallator_seed_matrix PRIVATE source)
add_test(NAME metallator_seed_matrix COMMAND metallator_seed_matrix)
add_executable(metallator_bench_core tools/bench_core.cpp)
target_include_directories(metallator_bench_core PRIVATE source)
add_executable(metallator_render tools/offline_render.cpp)
target_include_directories(metallator_render PRIVATE source)

option(METALLATOR_BUILD_VST3 "Build the instrument with the Steinberg VST3 SDK" ON)
if(METALLATOR_BUILD_VST3)
    include(FetchContent)
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG v3.8.1_build_84
        GIT_SHALLOW FALSE
        GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(vst3sdk)
    smtg_add_vst3plugin(125A_Metallator
        source/metal_synth.h
        source/impact_body.h
        source/mechanical_body.h
        source/metallatorids.h
        source/metallatorstate.h
        source/metallatorcontroller.h
        source/metallatorcontroller.cpp
        source/metallatorprocessor.h
        source/metallatorprocessor.cpp
        source/factory.cpp
        source/version.h)
    target_compile_definitions(125A_Metallator PRIVATE SMTG_USE_STDATOMIC_H=1)
    # VSTGUI uses Steinberg::getPlatformModuleHandle() from moduleinit.cpp.
    # Supply the SDK's actual Windows DLL entry point (sets moduleHandle to HINSTANCE);
    # do not fake a nullptr module handle or duplicate entry exports.
    if(SMTG_WIN)
        target_sources(125A_Metallator PRIVATE ${SDK_ROOT}/public.sdk/source/main/dllmain.cpp)
    endif()
    target_link_libraries(125A_Metallator PRIVATE sdk vstgui_support)
    smtg_target_add_plugin_resources(125A_Metallator RESOURCES resource/metallator.uidesc)
    smtg_target_configure_version_file(125A_Metallator)
    if(SMTG_WIN)
        target_compile_definitions(125A_Metallator PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()
endif()
