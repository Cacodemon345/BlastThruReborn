cmake_minimum_required( VERSION 3.1.0 )

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()

project(BlastThruReborn)

if(ANDROID)
#    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH} ./androidroot/")
endif()

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

option(BTR_USE_SDL "Build with SDL2 backend instead of SFML." OFF)

find_package(SndFile REQUIRED)
# SFML on OpenBSD causes the application to be killed when creating the window if the
# X11 and Xrandr library isn't explicitly linked due to missing symbols at runtime.
if (NOT (CMAKE_SYSTEM_NAME MATCHES "Windows" OR CMAKE_SYSTEM_NAME MATCHES "Darwin" OR ANDROID OR HAIKU))
    find_package(X11 REQUIRED)
else()
    set(X11_X11_LIB "")
    set(X11_Xrandr_LIB "")
    set(X11_FOUND 0)
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

if(BTR_USE_SDL)
    find_package(SDL2 REQUIRED CONFIG)
else()
    find_package(SFML 2.5 COMPONENTS graphics system window REQUIRED)
endif()
find_package(OpenAL REQUIRED)
if(ANDROID)
	add_library(BlastThruReborn "${BTR_PROJECT_FILES}")
else()
	add_executable(BlastThruReborn "${BTR_PROJECT_FILES}")
endif()
if (CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(BlastThruReborn PRIVATE "/Zc:__cplusplus" "/EHsc")
endif()
if (NOT BTR_USE_SDL)
    target_link_libraries(BlastThruReborn sfml-graphics sfml-system sfml-window)
else()
    target_link_libraries(BlastThruReborn SDL2::SDL2)
    target_include_directories(BlastThruReborn PUBLIC ${SDL2_INCLUDE_DIRS})
    target_compile_definitions(BlastThruReborn PRIVATE -DBTR_USE_SDL)
endif()
target_link_libraries(BlastThruReborn ${OPENAL_LIBRARY} ${SNDFILE_LIBRARIES})
if (X11_FOUND)
    target_link_libraries(BlastThruReborn ${X11_X11_LIB} ${X11_Xrandr_LIB})
endif()
if (HAIKU)
    target_link_libraries(BlastThruReborn media)
endif()
if (NOT (CMAKE_SYSTEM_NAME MATCHES "Windows"))
    if (NOT USE_RTMIDI17)
        target_link_libraries(BlastThruReborn ${FLUIDSYNTH_LIBRARIES} -lpthread)
        target_include_directories(BlastThruReborn PUBLIC ${FLUIDSYNTH_INCLUDE_DIR})
    else()
        target_link_libraries(BlastThruReborn libremidi)
        target_include_directories(BlastThruReborn PUBLIC ./RtMidi17/ ./RtMidi17/include/)
    endif()
else()
    target_link_libraries(BlastThruReborn winmm opengl32)
endif()
target_include_directories(BlastThruReborn PUBLIC ${OpenAL_INCLUDE_DIR} ${SNDFILE_INCLUDE_DIR})
