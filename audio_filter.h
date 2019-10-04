#pragma once

class i_audio_filter
{
public:
	virtual float process(float sample) = 0;
};