# Blast Thru Reborn

A written-from-scratch game engine for running Blast Thru (the eGames one).

Copyright (c) 2020 Cacodemon345 and contributors.

See the SimpleIni.h file and ConvertUTF.h file for the individual copyright notices.

# Building
Prerequisites:
1. OpenAL Soft (https://github.com/kcat/openal-soft).
2. libsndfile (http://www.mega-nerd.com/libsndfile/#Download).
3. SFML (2.5.1 or later, https://en.sfml-dev.org/download.php).
4. RtMidi17 (https://github.com/jcelerier/RtMidi17, needed when compiling on POSIX systems).
5. JACK Audio Connection Kit (should you want music support on the BSDs (installing one from the ports collection should be sufficient), https://jackaudio.org/).

Windows:
Modify the included Visual Studio project files to point to the SFML SDK (or the include folder from the source itself) and other necessary dependencies and then build the solution. Make sure the library paths are correct too.

Linux:
Install the necessary dependencies listed here (preferably from your distro's package manager) and run makelinux.sh like this: `. makelinux.sh`. Commands must be ran as non-root from the shell. Note that you will need to clone the RtMidi17 git repo inside this project's folder. Substitute "makelinux.sh" with "makelinux-clang.sh" if you want to build with the Clang compiler.

BSD:
Ditto, but run makebsd.sh instead like this: `. makebsd.sh`. If you get complaints about not being able to find the file (happens on OpenBSD), run makebsd.sh like this: `. ./makebsd.sh`. Commands must be ran as non-root from the shell. Note that extracting the game assets required for running the game won't be possible from a BSD.

macOS:
I haven't been able to get a Mac computer to test this on macOS, so anything you do to compile it is on your own (including setting up the necessary compiler environments). You still need to install the dependencies by yourself. I may end up testing this on macOS should I manage to get my hand on a Macintosh computer (or if a ARM one, bonus points there too). You will be installing the dependencies by yourself anyway. Note that extracting the game assets required for running the game won't be possible from macOS.

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
9. Run the executable (type "./a.out" in the terminal on POSIX systems).
Alternatively, just put ball.glo on the executable folder and run the program with "extractdata" argument. This requires that you got GloDecrypt from https://github.com/Cacodemon345/GloDecrypt and put the GloDecrypt executable (named GloDecrypt.exe or GloDecrypt) on this project's executable folder.
In all cases, you must make sure the "art" and "sound" folder exists in the executable. The folders also must contain everything from the original Blast Thru installation's same folders. You won't be able to run the game otherwise.
You will be asked to select the MIDI device on first run.

Note that quite a bit of stuff in the program that the original game has remains unimplemented.