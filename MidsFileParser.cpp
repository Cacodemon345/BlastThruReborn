#include "BTRCommon.h"
#pragma pack(push,1)
HMIDIOUT midiDev;
struct MidsDataHeader
{
	char FourCC[4];
	uint32_t chunkSize;
	uint32_t blocks;
};
struct MidsEvent
{
	DWORD dwDeltaTime;
	DWORD dwEvent;
};
struct MidsStreamIDEvent
{
	DWORD dwDeltaTime;
	DWORD dwStreamID;
	DWORD dwEvent;
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
MIDIHDR* hdr = NULL;
bool shouldContinue = false;
uint32_t swap_uint32(uint32_t val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}
void CALLBACK MidiCallBack(HMIDIOUT hmo, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (wMsg)
	{
	default:
		break;
	case MOM_DONE:
		shouldContinue = true;
		break;
	}
}
bool eot = false;
int tempoPerBeat = 6000000;
int timeDiv = 96;
std::chrono::steady_clock curClock;
void QueueMidiEvent(MidsEvent midsevent)
{
	//std::cout << "Delta time: " << midsevent.dwDeltaTime << std::endl;

	//usleep(tempoPerBeat / timeDiv * midsevent.dwDeltaTime);
	auto time = curClock.now().time_since_epoch().count();
	while ((curClock.now().time_since_epoch().count() - time) <= (tempoPerBeat * 1000) / timeDiv * midsevent.dwDeltaTime)
	{

	}
	//std::this_thread::sleep_for(std::chrono::duration<unsigned long long,std::micro>(tempoPerBeat / timeDiv * midsevent.dwDeltaTime));
	if (midsevent.dwEvent & (MEVT_TEMPO << 24))
	{
		tempoPerBeat = midsevent.dwEvent & 0x00FFFFFF;
		return;
	}
	midiOutShortMsg(midiDev, midsevent.dwEvent);
}
void QueueMidiEvent(MidsStreamIDEvent midsevent)
{
	MidsEvent realMidsEvent{ midsevent.dwDeltaTime, midsevent.dwEvent };
	return QueueMidiEvent(realMidsEvent);
}
void ParseMidsFile(std::string filename)
{
	hdr = (MIDIHDR*)HeapAlloc(GetProcessHeap(), 4 | 8, sizeof(*hdr));
	memset((void*)hdr, 0, sizeof(*hdr));
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
				}
				else
				{
					std::cout << "Total number of delta tick and MIDI message pair in this block: " << blockSize / sizeof(MidsEvent) << std::endl;
					MidsEvent* events = (MidsEvent*)calloc(blockSize / sizeof(MidsEvent), sizeof(MidsEvent));
					file.read((char*)events, blockSize);
					for (int i = 0; i < blockSize / sizeof(MidsEvent); i++)
					{
						MidsEvents.push_back(events[i]);
					}
				}
			}
		}
	}
}

void StartMidiPlayback()
{
	unsigned int devID = 0;
	midiOutOpen(&midiDev, devID, 0, 0, 0);
	while (1)
	{
		if (header.dwFlags == 0)
		{
			for (int i = 0; i < MidsStreamIDEvents.size(); i++)
			{
				QueueMidiEvent(MidsStreamIDEvents[i]);
				if (eot) return;
			}
		}
		else for (int i = 0; i < MidsEvents.size(); i++)
		{
			QueueMidiEvent(MidsEvents[i]);
			if (eot) return;
		}
	}
}