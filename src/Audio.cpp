#include "Audio.h"
#include <cstdlib>
#include <fmod.hpp>
#include <stdexcept>
#include <Windows.h>
Audio::Audio()
{

	result = FMOD::System_Create(&system);
	if (result != FMOD_OK)
	{
		throw std::runtime_error("ERROR: FMOD failed to initialize");
	}
	
	result = system->init(32, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)
	{
		throw std::runtime_error("ERROR: FMOD::System failed to initialize");
	}

	result = system->createSound("res/audio/Ohne Dich.mp3", FMOD_DEFAULT, 0, &sound);
	if (result != FMOD_OK)
	{
		throw std::runtime_error("ERROR: FMOD failed to create sound");
	}

	result = sound->setMode(FMOD_LOOP_OFF);

	if (result != FMOD_OK)
	{
		throw std::runtime_error("ERROR: FMOD failed to set mode");
	}
	playSound();
	changeVolume();
}

void Audio::playSound()
{
	system->playSound(sound, 0, false, &channel);
}

void Audio::Update()
{
	result = system->update();
}

void Audio::changeVolume()
{
	result = channel->setVolume(volume);
}

Audio::~Audio()
{
	sound->release();
	system->close();
	system->release();
	
}