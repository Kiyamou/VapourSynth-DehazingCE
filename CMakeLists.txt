cmake_minimum_required(VERSION 2.8)
project(DehazingCE)

set(CMAKE_BUILD_TYPE "Release")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(VAPOURSYNTH_INCLUDE_DIR "/usr/include" CACHE PATH "VapourSynth header files")
    if (NOT EXISTS "${VAPOURSYNTH_INCLUDE_DIR}/vapoursynth/VSHelper.h")
        message(FATAL_ERROR "VapourSynth header files not found, specify with -DVAPOURSYNTH_INCLUDE_DIR")
    endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(VAPOURSYNTH_INCLUDE_DIR "C:/Program Files/VapourSynth/sdk/include" CACHE PATH "VapourSynth header files")
    if (NOT EXISTS "${VAPOURSYNTH_INCLUDE_DIR}/vapoursynth/VSHelper.h")
        message(FATAL_ERROR "VapourSynth header files not found, specify with -DVAPOURSYNTH_INCLUDE_DIR")
    endif()
endif()

add_library(DehazingCE SHARED src/main.cpp src/DehazingCE.cpp src/GuidedFilter.cpp src/Lut.cpp)
target_include_directories(DehazingCE PRIVATE ${VAPOURSYNTH_INCLUDE_DIR})