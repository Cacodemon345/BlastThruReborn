# Having to deal with the macOS's ass development tools is annoying as fuck.
# Wish someone else had the balls to maintain macOS support for the game engine...
clang++ -std=gnu++17 MidsFileParser.cpp BTRPlayArea.cpp ConsoleApplication9.cpp  SoundPlayback.cpp -lsfml-graphics -lsfml-system -lsfml-window -lstdc++ -lm -lpthread -lsndfile -lopenal -framework CoreMIDI -framework CoreFoundation -framework CoreAudio -g -I ./  -I ./RtMidi17/ -I ./RtMidi17/include/ -L/usr/local/opt/openal-soft/lib -I/usr/local/opt/openal-soft/include -DRTMIDI17_COREAUDIO
