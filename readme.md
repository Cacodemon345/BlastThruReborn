# Blast Thru Reborn

A written-from-scratch game engine for running Blast Thru (the eGames one).

Copyright (c) 2020-2021 Cacodemon345 and contributors.

See the SimpleIni.h file and ConvertUTF.h file for the individual copyright notices.

# Building
Prerequisites:
1. OpenAL Soft (https://github.com/kcat/openal-soft).
2. libsndfile (http://www.mega-nerd.com/libsndfile/#Download).
3. SFML (2.5.1 or later, https://en.sfml-dev.org/download.php).
4. RtMidi17 (https://github.com/jcelerier/RtMidi17, needed when compiling on POSIX systems, optional).
5. JACK Audio Connection Kit (should you want music support on the BSDs (installing one from the ports collection should be sufficient), https://jackaudio.org/, needed when compiling on non-Linux Unix-like systems, optional if not using RtMidi17).
6. FluidSynth (https://github.com/FluidSynth/fluidsynth, needed when compiling on POSIX systems, default)
7. A C++17 supporting compiler (C++20 or later recommended). On Windows, only Visual Studio 2019 or later is supported.

Only 64-bit builds are supported. This game engine works on both x86-64 (known as x64 on Windows) and ARM64 systems.

Clone this repo:
```git clone --recurive https://github.com/Cacodemon345/BlastThruReborn```

And run CMake like this, assuming you have all the prerequisites installed:

```cmake -S ./ -B ./build/```

Append `-DUSE_RTMIDI17=1` to use RtMidi17 instead of FluidSynth on non-Windows systems.

```cmake --build ./build```

Add `--parallel numofcores` parameter to the invocation of above, where `numofcores` is the number of cores/processors your computer has to speed up building.

vcpkg is required to build this on Windows. If you set the VCPKG_ROOT environment variable to the path containing your vcpkg installation the CMake script will automatically find the toolchain file and use it. Alternatively, you can also specify the vcpkg toolchain file manually via command line.

# Running
The repository does not contain the game assets for obvious reasons. You must own the game. To get this program running:
1. Download Blast Thru Editor from https://www.moddb.com/games/blast-thru/downloads/blast-thru-editor-1600.
2. Copy the editor files (including the executable) into your Blast Thru folder.
3. Launch the editor (using Wine if using Linux).
4. Configure the editor to locate the bt.exe file.
5. Now open the "Advanced" menu. Find the option to open the ball.glo file.
6. Extract the files to the Blast Thru Reborn installation.
7. Rename the "ball.glo" folder to "ball".
8. Build the program as described in the Building section.
9. Run the executable (type the path to the BlastThruReborn executable you built in the terminal on POSIX systems).

Alternatively, just put ball.glo on the executable folder and run the program with "extractdata" argument. This requires that you got GloDecrypt from https://github.com/Cacodemon345/GloDecrypt and put the GloDecrypt executable (named GloDecrypt.exe or GloDecrypt) on this project's executable folder, assuming you followed the requirements to build that project.
In all cases, you must make sure the "art" and "sound" folder exists in the executable. The folders also must contain everything from the original Blast Thru installation's same folders. You won't be able to run the game otherwise.
You will be asked to select the MIDI device on first run.

Note that quite a bit of stuff in the program that the original game has remains unimplemented.