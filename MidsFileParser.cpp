#include "BTRCommon.h"
#if defined(WIN32)
#pragma pack(push,1)
HMIDISTRM midiDev;
std::string curFilename;
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
HANDLE evthandle;
bool shouldContinue = false;
bool shouldStartPlayback = false;
bool eot = false;
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
std::vector<MIDIHDR> midiHeaders;
std::string& GetCurPlayingFilename()
{
	return curFilename;
}
void ParseMidsFile(std::string filename)
{
	unsigned int id = 0;
	for (auto& hdr : midiHeaders)
	{
		delete[] hdr.lpData;
		hdr.lpData = 0;
	}
	midiHeaders.clear();
	curFilename = filename;
	std::fstream file = std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		auto size = file.tellg();
		file.seekg(0, file.beg);
		file.read((char*)&header, sizeof(header));
		if (strncmp(header.RIFFFourCC, "RIFF", 4) == 0 && strncmp(header.FourCC, "MIDS", 4) == 0
			&& strncmp(header.fmtFourCC, "fmt ", 4) == 0 && header.headerSize == 12)
		{
			MIDIPROPTIMEDIV timeDiv;
			timeDiv.cbStruct = sizeof(timeDiv);
			timeDiv.dwTimeDiv = header.dwTimeFormat;
			midiStreamProperty(midiDev, (LPBYTE)&timeDiv, MIDIPROP_SET | MIDIPROP_TIMEDIV);
			//std::cout << "MDS Time Division: " << header.dwTimeFormat << std::endl;
		}
		else
		{
			std::cout << "Bad MIDS file header" << std::endl;
			return;
		}
		file.read((char*)&dataHeader, sizeof(dataHeader));
		uint8_t* buffer;
		while (file.tellg() != EOF)
		{
			uint32_t tickOffset = 0;
			uint32_t blockSize = 2728;
			while (!file.eof())
			{
				file.read((char*)&tickOffset, 4);
				file.read((char*)&blockSize, 4);
				if (file.eof())
				{
					file.close();
					return;
				}
				//std::cout << "Current Block Size: " << blockSize << std::endl;
				//std::cout << "Found at offset: " << file.tellg() << std::endl;
				//std::cout << "Current tick offset: " << tickOffset << std::endl;
				buffer = new uint8_t[blockSize];
				auto curFileOffset = file.tellg();
				if (header.dwFlags == 0) // 0 means Stream ID exists in the MDS File.
				{
					file.read((char*)buffer, blockSize);
					MIDIHDR header;
					header.dwBytesRecorded = header.dwBufferLength = blockSize;
					header.lpData = (LPSTR)buffer;
					header.dwFlags = 0;
					midiHeaders.push_back(header);
					buffer = 0;
				}
				else
				{
					std::cerr << "Stream ID needs to exist in the messages!" << std::endl;
				}
			}
		}
	}
	else
	{
		std::cout << "Invalid Filename" << std::endl;
	}
}
extern void StopMidiPlayback();
void StartMidiPlayback()
{
	midiStreamRestart(midiDev);
	while (1)
	{
		for (auto& hdr : midiHeaders)
		{
			if (!(hdr.dwFlags & MHDR_PREPARED))midiOutPrepareHeader((HMIDIOUT)midiDev, &hdr, sizeof(hdr));
			auto err = midiStreamOut(midiDev, &hdr, sizeof(hdr));
			shouldContinue = false;
			if (err != MMSYSERR_NOERROR)
			{
				std::cout << "Failed to play MIDI buffer\n";
				return;
			}
			while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader((HMIDIOUT)midiDev, &hdr, sizeof(hdr)))
			{
				if (eot)
				{
					midiStreamStop(midiDev);
					return;
				}
			}
		}
		ParseMidsFile(curFilename);
	}
	midiStreamStop(midiDev);
}

void StopMidiPlayback()
{
	midiStreamStop(midiDev);
	midiStreamClose(midiDev);
}
#else
bool eot = false;
void ParseMidsFile(std::string filename)
{

}

void StartMidiPlayback()
{
	while (1) if (eot) return;
}

void StopMidiPlayback()
{

}
#endif
