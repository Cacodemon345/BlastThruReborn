cc -std=gnu++17 MidsFileParser.cpp BTRPlayArea.cpp ConsoleApplication9.cpp ConvertUTF.c  SoundPlayback.cpp -lgcc -lSDL2 -lstdc++ -lm -lpthread -lasound -lopenal -lGL -lsndfile -lrt -g -I ./ -I ./RtMidi17/ -I ./RtMidi17/include/ -DRTMIDI17_ALSA -DBTR_USE_SDL -DRTMIDI17_HEADER_ONLY -o BlastThruReborn -lSDL2_gpu -L /usr/local/lib
