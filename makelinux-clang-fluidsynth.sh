clang++ -std=gnu++17 FlSynthMidsFileParser.cpp BTRPlayArea.cpp ConsoleApplication9.cpp  SoundPlayback.cpp -lgcc -lsfml-graphics -lsfml-system -lsfml-window -lstdc++ -lm -lpthread -lasound -lopenal -lGL -lsndfile -lfluidsynth -lrt -g -I ./  -I ./RtMidi17/ -DRTMIDI17_ALSA