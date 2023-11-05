#include "Audio.h"
#include <cstdlib>
#include <fmod.hpp>
#include <stdexcept>

Audio::Audio()
{
	if (FMOD::Studio::System::create(&sys, FMOD_VERSION) != FMOD_OK)
	{
		throw std::runtime_error("ERROR: Failed to create FMOD System!");
	}
	
}

Audio::~Audio()
{
	sys->release();
	free(this);
}