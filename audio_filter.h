#pragma once

class i_audio_filter
{
public:
	virtual float process(float sample) = 0;
	virtual ~i_audio_filter() {};
};