#ifndef __AUDIO_CLASS__
#define __AUDIO_CLASS__
#include <cstdlib>
#include <fmod_studio.hpp>
#include <fmod_common.h>
class Audio
{
public:
	Audio();
	~Audio();
	void Update();
	void playSound();
	void changeVolume();
	float volume = 0.000f;
	
private:
	FMOD::System * system;
	FMOD::Sound* sound;
	FMOD::Channel* channel = 0;
	FMOD_RESULT result;
};



#endif
