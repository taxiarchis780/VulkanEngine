#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <fmod_studio.hpp>

class Audio
{
public:
	Audio();
	~Audio();
private:
	FMOD::Studio::System* sys;
};



#endif
