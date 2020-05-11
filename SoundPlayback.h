#pragma once
#include <string>

bool InitOpenAL();
void BTRPlaySound(std::string filename, bool looping = false, bool playOnSameChannel = false, bool isMusic = false, bool queueUp = false);
void BTRPlaySound(const char* filename, bool looping = false, bool playOnSameChannel = false,bool isMusic = false,bool queueUp = false);
void BTRStopAllSounds();
void deInitOpenAL();
