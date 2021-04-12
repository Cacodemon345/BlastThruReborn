cmake_minimum_required( VERSION 3.1.0 )

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()
project(BlastThruReborn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

if (NOT (CMAKE_SYSTEM_NAME MATCHES "Windows"))
    option(USE_RTMIDI17 "Use RtMidi17 instead of Fluidsynth" OFF)
    if (USE_RTMIDI17)
        # Disable building RtMidi17 examples by default.
        option(RTMIDI17_EXAMPLES "Enable examples" OFF)
        add_subdirectory("./RtMidi17")
    else()
        find_package(FluidSynth REQUIRED)
    endif()
endif()

find_package(SndFile REQUIRED)
# SFML on OpenBSD causes the application to be killed when creating the window if the
# X11 and Xrandr library isn't explicitly linked due to missing symbols at runtime.
if (NOT (CMAKE_SYSTEM_NAME MATCHES "Windows" OR CMAKE_SYSTEM_NAME MATCHES "Darwin"))
    find_package(X11 REQUIRED)
else()
    set(X11_X11_LIB "")
    set(X11_Xrandr_LIB "")
endif()

set( BTR_PROJECT_FILES BTRPlayArea.cpp ConsoleApplication9.cpp  SoundPlayback.cpp ConvertUTF.c )

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set( BTR_PROJECT_FILES MidsFileParser.cpp ${BTR_PROJECT_FILES})
elseif(USE_RTMIDI17)
    set( BTR_PROJECT_FILES AltMidsFileParser.cpp ${BTR_PROJECT_FILES})
elseif(FLUIDSYNTH_FOUND)
    set( BTR_PROJECT_FILES FlSynthMidsFileParser.cpp ${BTR_PROJECT_FILES})
endif()

if (CMAKE_SYSTEM_NAME MATCHES "BSD")
    # Set -Wno-register to avoid errors during compilation.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register" )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-register" )
endif()

find_package(SFML 2.5 COMPONENTS graphics system window REQUIRED)
find_package(OpenAL REQUIRED)
add_executable(BlastThruReborn "${BTR_PROJECT_FILES}")
if (CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(BlastThruReborn PRIVATE "/Zc:__cplusplus")
endif()
target_link_libraries(BlastThruReborn sfml-graphics sfml-system sfml-window ${OPENAL_LIBRARY} ${SNDFILE_LIBRARIES} ${X11_X11_LIB} ${X11_Xrandr_LIB})
if (NOT (CMAKE_SYSTEM_NAME MATCHES "Windows"))
    if (NOT USE_RTMIDI17)
        target_link_libraries(BlastThruReborn ${FLUIDSYNTH_LIBRARIES} -pthread)
        target_include_directories(BlastThruReborn PUBLIC ${FLUIDSYNTH_INCLUDE_DIR})
    else()
        target_link_libraries(BlastThruReborn RtMidi17)
        target_include_directories(BlastThruReborn PUBLIC ./RtMidi17/ ./RtMidi17/include/)
    endif()
else()
    target_link_libraries(BlastThruReborn winmm)
endif()
target_include_directories(BlastThruReborn PUBLIC ${OpenAL_INCLUDE_DIR} ${SNDFILE_INCLUDE_DIR})