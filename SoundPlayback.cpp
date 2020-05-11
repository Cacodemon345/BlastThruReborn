#include "BTRCommon.h"
#include "SoundPlayback.h"
static ALCdevice* Device = NULL;
static ALCcontext* Context = NULL;
static ALuint* Buffers = (ALuint*)malloc(sizeof(ALuint) * 129);
static ALuint* source = (ALuint*)malloc(sizeof(ALuint) * 129);
static ALuint* musBuf = (ALuint*)malloc(sizeof(ALuint));
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
	alGenBuffers(129, Buffers);
	
	if ((error = alGetError()) != AL_NO_ERROR)
	{ 
		std::cout << "alGenBuffers failed" << std::endl;
		return false;
	}
	// Generate Sources
	alGenSources(129, source);
	OpenALInited = true;
	return true;
}
void BTRPlaySound(std::string filename, bool looping, bool playOnSameChannel, bool isMusic, bool queueUp)
{
	return BTRPlaySound(filename.c_str(), looping, playOnSameChannel, isMusic, queueUp);
}
void BTRPlaySound(const char* filename, bool looping, bool playOnSameChannel, bool isMusic, bool queueUp)
{
	if (!OpenALInited) return;
	SF_INFO info;
	info.format = 0;
	auto file = sf_open(filename, SFM_READ, &info);
	auto orgBuffer = curBuffer;
	if (file)
	{
		if (playOnSameChannel && curBuffer)
		{
			curBuffer--;
		}
		if (curBuffer >= 128)
		{
			curBuffer = 0;
		}
		if (isMusic)
		{
			curBuffer = 128;
		}
		auto framedata = new short[info.frames * info.channels];
		//alSourceStop(source[curBuffer]);
		alSourcei(source[curBuffer], AL_BUFFER, NULL);		
		//alDeleteBuffers(1, &Buffers[curBuffer]);
		sf_readf_short(file, framedata, info.frames);		
		alBufferData(Buffers[curBuffer], AL_FORMAT_MONO16 + (info.channels == 2 ? 2 : 0), framedata, info.frames * info.channels * sizeof(short), info.samplerate);
		if (queueUp) 
		{
			ALint isPlaying = false;
			alGetSourcei(source[curBuffer], AL_SOURCE_STATE, &isPlaying);
			if (isPlaying != AL_PLAYING) alSourcei(source[curBuffer], AL_BUFFER, Buffers[curBuffer]);
			else 
			{
				delete[] framedata;
				sf_close(file);
				curBuffer = ++orgBuffer;
				return;
			}
		}
		else alSourcei(source[curBuffer], AL_BUFFER, Buffers[curBuffer]);
		if (looping) alSourcei(source[curBuffer], AL_LOOPING, AL_TRUE);
		else alSourcei(source[curBuffer], AL_LOOPING, AL_FALSE);
		alSourcePlay(source[curBuffer]);
		curBuffer++;
		delete[] framedata;
		sf_close(file);
		if (isMusic) curBuffer = ++orgBuffer;
	}
}
void BTRStopAllSounds()
{
	for (int i = 0; i < 129; i++)
	{
		alSourcei(source[i], AL_LOOPING, AL_FALSE);
		alSourceStop(source[i]);
	}
}
void deInitOpenAL()
{
	if (!OpenALInited) return;
	Context = alcGetCurrentContext();
	Device = alcGetContextsDevice(Context);
	BTRStopAllSounds();
	alDeleteBuffers(129, Buffers);
	alDeleteSources(129, source);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(Context);
	alcCloseDevice(Device);
}