#pragma once
#define DIRECTSOUND_VERSION 0x1000
#include "dsound.h"
#pragma comment (lib, "dsound.lib")
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <cstdint>

class c_sound
{
public:
	c_sound(HWND hWnd);
	~c_sound();
	int init();
	void play();
	void stop();
	int copy(const int32_t *src, int numSamples);
	double get_freq() { return freq; }
	int get_max_freq() { return max_freq; }
	int rate;
	void set_volume(long volume);
	int sync();
	int resets;
	void clear();
	int max_freq;
	int min_freq;
	double slope;
	double get_requested_freq();
	void reset();
private:
	double requested_freq;

	HWND hWnd;
	double freq;
	int adjustPeriod;
	int lastb;
	short *buf;
	int nSamples;
	static const int BUFFER_MSEC=140;
	static const int adjustFrames=3;
	static const int target=48000*2/60*3*2;//1600*3;
	WAVEFORMATEX wf;
	LPDIRECTSOUND lpDS;
	LPDIRECTSOUNDBUFFER buffer, primaryBuffer;
	DSBUFFERDESC bufferdesc, primaryBufferDesc;
	DWORD write_cursor;
	int get_max_write();
	double calc_slope();

	int default_max_freq;
	int default_min_freq;
	int default_freq;

	static const int num_values = 60;
	int values[num_values];
	int freq_values[num_values];
	int value_index;

	double ema;
	int first_b;
};
