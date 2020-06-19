#pragma once
#include <string>

bool InitOpenAL();
void BTRPlaySound(std::string filename, bool looping = false, bool playOnSameChannel = false, bool isMusic = false, bool queueUp = false, float sourceX = 0, float sourceY = 0, float sourceZ = 0);
void BTRPlaySound(const char* filename, bool looping = false, bool playOnSameChannel = false,bool isMusic = false,bool queueUp = false, float sourceX = 0, float sourceY = 0, float sourceZ = 0);
void BTRStopAllSounds();
void deInitOpenAL();
