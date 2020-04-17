#include "BTRCommon.h"
#include "SoundPlayback.h"
static ALCdevice* Device = NULL;
static ALCcontext* Context = NULL;
static ALuint* Buffers = (ALuint*)malloc(sizeof(ALuint) * 128);
static ALuint* source = (ALuint*)malloc(sizeof(ALuint) * 128);
bool OpenALInited = false;
int curBuffer = 0;
bool InitOpenAL()
{
	// Initialization
	Device = alcOpenDevice(NULL); // select the "preferred device" 
	if (Device)
	{
		Context = alcCreateContext(Device, NULL);
		alcMakeContextCurrent(Context);
	}
	auto error = alGetError(); // clear error code 
	alGenBuffers(128, Buffers);
	
	if ((error = alGetError()) != AL_NO_ERROR)
	{ 
		std::cout << "alGenBuffers failed" << std::endl;
		return false;
	}
	// Generate Sources
	alGenSources(128, source);
	OpenALInited = true;
	return true;
}
void BTRPlaySound(const char* filename)
{
	if (!OpenALInited) return;
	SF_INFO info;
	info.format = 0;
	auto file = sf_open(filename, SFM_READ, &info);
	if (file)
	{
		if (curBuffer >= 128)
		{
			curBuffer = 0;
		}
		auto framedata = new short[info.frames * info.channels];
		alSourceStop(source[curBuffer]);
		alSourcei(source[curBuffer], AL_BUFFER, NULL);
		//alDeleteBuffers(1, &Buffers[curBuffer]);
		sf_readf_short(file, framedata, info.frames);
		alBufferData(Buffers[curBuffer], AL_FORMAT_MONO16, framedata, info.frames * info.channels * sizeof(short), info.samplerate);
		alSourcei(source[curBuffer], AL_BUFFER, Buffers[curBuffer]);
		alSourcePlay(source[curBuffer]);
		curBuffer++;
	}
}

void deInitOpenAL()
{
	if (!OpenALInited) return;
	Context = alcGetCurrentContext();
	Device = alcGetContextsDevice(Context);
	for (int i = 0; i < 128; i++)
	{
		alSourceStop(source[i]);
	}
	alDeleteBuffers(128, Buffers);
	alDeleteSources(128, source);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(Context);
	alcCloseDevice(Device);
}