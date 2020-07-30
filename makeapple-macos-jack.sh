# Alternate macOS compile script in case Apple drops CoreMIDI support in their OS. May or may not work.
clang++ -std=gnu++2a MidsFileParser.cpp BTRPlayArea.cpp ConsoleApplication9.cpp  SoundPlayback.cpp -lsfml-graphics -lsfml-system -lsfml-window -lstdc++ -lm -lpthread -lsndfile -lopenal -g -I ./  -I ./RtMidi17/ -L/usr/local/opt/openal-soft/lib -I/usr/local/opt/openal-soft/include -DRTMIDI17_JACK
