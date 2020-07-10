#include <iostream>
//#include <windows.h>
//#include <mmsystem.h>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <fstream>
//#include <tchar.h>
#define RTMIDI17_HEADER_ONLY
#include "rtmidi17/rtmidi17.hpp"
//HMIDIOUT midiDev;
extern void ParseMidsFile(std::string filename);
extern void StartMidiPlayback();
rtmidi::midi_out midiOut;
extern bool eot;
extern unsigned int devID;
#ifdef BTRMID_STANDALONE
unsigned int devID = 0;
#endif
bool paused = 0;
bool oneshotplay = false;
#pragma pack(push,1)
struct MidsDataHeader
{
	char FourCC[4];
	uint32_t chunkSize;
	uint32_t blocks;
};
struct MidsEvent
{
	int dwDeltaTime;
	unsigned int dwEvent;
};
struct MidsStreamIDEvent
{
	int dwDeltaTime;
	int dwStreamID;
	unsigned int dwEvent;
};
std::vector<MidsEvent> MidsEvents;
std::vector<MidsStreamIDEvent> MidsStreamIDEvents;
struct MidsHeader
{
	char RIFFFourCC[4];
	uint32_t RIFFFileSize;
	char FourCC[4];
	char fmtFourCC[4];
	uint32_t headerSize;
	uint32_t dwTimeFormat;
	uint32_t cbMaxBuffer;
	uint32_t dwFlags;
};

#pragma pack(pop)
MidsHeader header;
MidsDataHeader dataHeader;
bool shouldContinue = false;

bool eot = false;
int tempoPerBeat = 6000000;
int timeDiv = 96;
std::chrono::steady_clock curClock;
void SelectMidiDevice(int selection)
{
	if (selection >= midiOut.get_port_count())
	{
		selection = 0;
	}
	midiOut.open_port(selection);
	devID = selection;
}
void SelectMidiDevice()
{
    auto cnt = midiOut.get_port_count();
    for (uint32_t i = 0; i < cnt; i++)
    {
        std::cout << i << ". " << midiOut.get_port_name(i) << std::endl;
    }
    std::cout << "Select MIDI device: " << std::endl;
    int selection;
    std::cin >> selection;
	devID = selection;
    midiOut.open_port(selection);
}
#ifdef BTRMID_STANDALONE

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		if (strncmp(argv[1],"exportmid",9))
		{
			
		}
	}
	std::cout << "Select MIDS file: " << std::endl;
	std::string str;
	std::cin >> str;
	SelectMidiDevice();
	ParseMidsFile(str);
	StartMidiPlayback();
}
#endif
void QueueMidiEvent(MidsEvent midsevent)
{
	//std::cout << "Delta time: " << midsevent.dwDeltaTime << std::endl;
	while(paused) {}
	//usleep(tempoPerBeat / timeDiv * midsevent.dwDeltaTime);
	auto time = curClock.now().time_since_epoch().count();
	while ((curClock.now().time_since_epoch().count() - time ) <= (tempoPerBeat * 1000) / timeDiv * midsevent.dwDeltaTime)
	{
        if (eot) return; // Cancel counting immediatly.
	}
	//std::this_thread::sleep_for(std::chrono::duration<unsigned long long,std::micro>(tempoPerBeat / timeDiv * midsevent.dwDeltaTime));
	if (midsevent.dwEvent & (1 << 24))
	{
		tempoPerBeat = midsevent.dwEvent & 0x00FFFFFF;
		std::cout << "New tempo/beat in microseconds: " << tempoPerBeat << std::endl;
		return;
	}
	uint8_t bytes = 3;
	if ((midsevent.dwEvent & 0xC0) == 0xC0
		|| (midsevent.dwEvent & 0xD0) == 0xD0) bytes = 2;
	midiOut.send_message((unsigned char*)&midsevent.dwEvent,bytes);
}
void QueueMidiEvent(MidsStreamIDEvent midsevent)
{
	MidsEvent realMidsEvent{ midsevent.dwDeltaTime, midsevent.dwEvent};
	return QueueMidiEvent(realMidsEvent);
}
std::string curFilename;
std::string& GetCurPlayingFilename()
{
    return curFilename;
}
void ParseMidsFile(std::string filename)
{
	//hdr = (MIDIHDR*)HeapAlloc(GetProcessHeap(), 4 | 8, sizeof(*hdr));
	//memset((void*)hdr, 0, sizeof(*hdr));
    for (unsigned char i = 0; i < 16; i++)
    {
        unsigned char firstByte = 0xB0;
        firstByte |= i;
        std::vector<unsigned char> msgVec = {firstByte,0x7B,0};
        midiOut.send_message(msgVec);
    }
	curFilename = filename;
    paused = false;
    eot = false;
    /*for (int i = 0; i < 16; i++)
    {
        for (int ii = 0; ii < 128; ii++)
        midiOut.send_message(rtmidi::message::note_off(i,ii,0));
    }*/
    MidsEvents.clear();
    MidsStreamIDEvents.clear();
    std::fstream file = std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		auto size = file.tellg();
		file.seekg(0, file.beg);
		file.read((char*)&header, sizeof(header));
		if (strncmp(header.RIFFFourCC, "RIFF", 4) == 0 && strncmp(header.FourCC, "MIDS", 4) == 0
			&& strncmp(header.fmtFourCC, "fmt ", 4) == 0 && header.headerSize == 12)
		{
			timeDiv = header.dwTimeFormat;
			std::cout << "MDS Time Division: " << header.dwTimeFormat << std::endl;
		}
		else
		{
			std::cout << "Bad MIDS file header" << std::endl;
			return;
		}
		file.read((char*)&dataHeader, sizeof(dataHeader));
		while (file.tellg() != EOF)
		{
			uint32_t tickOffset = 0;
			uint32_t blockSize = 2728;
			for (int i = 0; i < dataHeader.blocks; i++)
			{
				file.read((char*)&tickOffset, 4);
				file.read((char*)&blockSize, 4);
				if (file.eof()) return; // End of parsing here.
				std::cout << "Current Block Size: " << blockSize << std::endl;
				std::cout << "Found at offset: " << file.tellg() << std::endl;
				std::cout << "Current tick offset: " << tickOffset << std::endl;
				if (header.dwFlags == 0) // 0 means Stream ID exists in the MDS File.
				{
					std::cout << "Total number of delta tick, StreamID and MIDI message pair in this block: " << blockSize / sizeof(MidsStreamIDEvent) << std::endl;
					MidsStreamIDEvent* events = (MidsStreamIDEvent*)calloc(blockSize / sizeof(MidsStreamIDEvent), sizeof(MidsStreamIDEvent));
                    file.read((char*)events, blockSize);
					for (int i = 0; i < blockSize / sizeof(MidsStreamIDEvent); i++)
					{
						MidsStreamIDEvents.push_back(events[i]);
					}
					//free(events);
				}
				else
				{
					std::cout << "Total number of delta tick and MIDI message pair in this block: " << blockSize / sizeof(MidsEvent) << std::endl;
					MidsEvent* events = (MidsEvent*)calloc(blockSize / sizeof(MidsEvent), sizeof(MidsEvent));
					file.read((char*)events, blockSize);
                    if (blockSize % 4)
                    {
                        std::cout << "Warning: Misaligned block" << std::endl;
                    }
					for (int i = 0; i < blockSize / sizeof(MidsEvent); i++)
					{
						MidsEvents.push_back(events[i]);
					}
					//free(events);
				}
			}
		}
	}
	else
	{
		std::cout << "Error: Failed to open file!" << std::endl;
		#ifdef BTRMID_STANDALONE
		exit(-1);
		#endif
	}
}

void StartMidiPlayback()
{
    //midiOut.send_message(rtmidi::message::control_change(0,0x79,0));
    //midiOut.send_message(rtmidi::message::control_change(0,0x7B,0));
	unsigned int devID = 0;
#ifndef BTRMID_STANDALONE
    while(1)
#endif
	{
		if (header.dwFlags == 0)
		{
			for (int i = 0; i < MidsStreamIDEvents.size(); i++)
			{
				while(paused)
				{
					if (eot) return;
				}
				QueueMidiEvent(MidsStreamIDEvents[i]);
				if (eot) return;
			}
		}
		else for (int i = 0; i < MidsEvents.size(); i++)
		{
			while(paused)
			{
				if (eot) return;
			}
			QueueMidiEvent(MidsEvents[i]);
			if (eot) return;
		}
		if (oneshotplay) return;
	}
}
void StopMidiPlayback()
{
    paused = false;
	eot = true;
    //midiOut.send_message(rtmidi::message::control_change(0,0x79,0));
}
void PauseMidiPlayback()
{
    /*for (int i = 0; i < 16; i++)
    {
        for (int ii = 0; ii < 128; ii++){}
        //midiOut.send_message(rtmidi::message::note_off(i,ii,0));
    }*/
	for (unsigned char i = 0; i < 16; i++)
	{
		std::vector<unsigned char> midiMsg = {static_cast<unsigned char>(0xB0 | i),0x7B,0};
		midiOut.send_message(midiMsg);
	}
    paused = true;
}
void ContinueMidiPlayback()
{
    paused = false;
}
