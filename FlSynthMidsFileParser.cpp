//
// Created by caco345 on 9/4/20.
//
// FluidSynth interface based on the RtMidi17 interface.
//

#include <fluidsynth.h>
#include <string>
#include <cstring>
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>

extern void ParseMidsFile(std::string filename);
extern void StartMidiPlayback();
fluid_synth_t* player;
fluid_settings_t* settings;
fluid_audio_driver_t* audioDriver;
fluid_sequencer_t* sequencer;
fluid_seq_id_t seqId;
#ifdef __HAIKU__
#include <os/MediaKit.h>
static BSoundPlayer* sndplayer = nullptr;

static void BufProc(void* cookie, void *buffer, size_t size, const media_raw_audio_format &format)
{
    fluid_synth_write_float(player, size / 8, buffer, 0, 2, buffer, 1, 2);
}
#endif
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
std::string curFilename;
//MIDI enums.
enum
{
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    POLY_PRESSURE = 0xA0,
    CONTROL_CHANGE = 0xB0,
    PROGRAM_CHANGE = 0xC0,
    AFTERTOUCH = 0xD0,
    PITCH_BEND = 0xE0,
};
std::string& GetCurPlayingFilename()
{
    return curFilename;
}

void TerminateMidiPlayback()
{
#ifdef __HAIKU__
    sndplayer->SetHasData(false);
    sndplayer->Stop();
    delete sndplayer;
#else
    delete_fluid_audio_driver(audioDriver);
#endif
    delete_fluid_synth(player);
    delete_fluid_settings(settings);
}

void SelectMidiDevice()
{
    settings = new_fluid_settings();
    fluid_settings_setint(settings, "audio.period-size", 1024);
    fluid_settings_setint(settings, "audio.periods", 1);
    fluid_settings_setnum(settings, "synth.sample-rate", 48000);
    player = new_fluid_synth(settings);
#ifndef __HAIKU__
    audioDriver = new_fluid_audio_driver(settings,player);
#endif
    auto res = fluid_synth_sfload(player,"./soundfont.sf2",1); // This one's for the Unixers...
    if (res == FLUID_FAILED)
    {
        printf("Failed to load soundfont");
    }
 //   sequencer = new_fluid_sequencer2(0);
//    seqId = fluid_sequencer_register_fluidsynth(sequencer,player);
#ifdef __HAIKU__
    media_raw_audio_format format{};
    format.frame_rate = 48000;
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.buffer_size = 960 * 2 * 4;
    format.byte_order = B_MEDIA_LITTLE_ENDIAN;
    format.channel_count = 2;
    sndplayer = new BSoundPlayer(&format, NULL, BufProc, NULL, nullptr);
    sndplayer->Start();
    sndplayer->SetHasData(true);
#endif
}
void SelectMidiDevice(int selection)
{
    return SelectMidiDevice();
}
void ParseMidsFile(std::string filename)
{
    //hdr = (MIDIHDR*)HeapAlloc(GetProcessHeap(), 4 | 8, sizeof(*hdr));
    //memset((void*)hdr, 0, sizeof(*hdr));
    if (player)
    {
        fluid_synth_system_reset(player);
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
#ifdef BTRMID_STANDALONE
            exit(-1);
#endif
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
    int status = midsevent.dwEvent & 0xF0;
    int chan = midsevent.dwEvent & 0x0F;
    int parm1 = (midsevent.dwEvent & 0xFF00) >> 8;
    int parm2 = (midsevent.dwEvent & 0xFF0000) >> 16;
    switch(status)
    {
        case NOTE_ON:
            
            fluid_synth_noteon(player,chan,parm1,parm2);
            break;
        case NOTE_OFF:
            fluid_synth_noteoff(player,chan,parm1);
            break;
        case CONTROL_CHANGE:
            fluid_synth_cc(player,chan,parm1,parm2);
            break;
        case PROGRAM_CHANGE:
            fluid_synth_program_change(player,chan,parm1);
            break;
        case AFTERTOUCH:
            fluid_synth_channel_pressure(player,chan,parm1);
            break;
        case PITCH_BEND:
            fluid_synth_pitch_bend(player,chan,(parm1 & 0x7f) | ((parm2 & 0x7f) << 7));
            break;
    }
}
void QueueMidiEvent(MidsStreamIDEvent midsevent)
{
    MidsEvent realMidsEvent{ midsevent.dwDeltaTime, midsevent.dwEvent};
    return QueueMidiEvent(realMidsEvent);
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
    fluid_synth_cc(player,0,0x79,0);
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
        fluid_synth_cc(player,i,0x7B,0);
    }
    paused = true;
}
void ContinueMidiPlayback()
{
    paused = false;
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
