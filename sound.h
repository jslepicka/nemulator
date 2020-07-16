///////////////////////////////////////////////////////////////////////////////////
//                                                                               //
//   nemulator (an NES emulator)                                                 //
//                                                                               //
//   Copyright (C) 2003-2009 James Slepicka <james@nemulator.com>                //
//                                                                               //
//   This program is free software; you can redistribute it and/or modify        //
//   it under the terms of the GNU General Public License as published by        //
//   the Free Software Foundation; either version 2 of the License, or           //
//   (at your option) any later version.                                         //
//                                                                               //
//   This program is distributed in the hope that it will be useful,             //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of              //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               //
//   GNU General Public License for more details.                                //
//                                                                               //
//   You should have received a copy of the GNU General Public License           //
//   along with this program; if not, write to the Free Software                 //
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   //
//                                                                               //
///////////////////////////////////////////////////////////////////////////////////

#pragma once
//#include "mmsystem.h"
#define DIRECTSOUND_VERSION 0x1000
#include "dsound.h"
#pragma comment (lib, "dsound.lib")
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <cstdint>

class Sound
{
public:
	Sound(HWND hWnd);
	~Sound(void);
	int Init(void);
	void Play(void);
	void Stop(void);
	int Copy(const int32_t *src, int numSamples);
	double GetFreq(void) { return freq; }
	int GetMaxFreq(void) { return max_freq; }
	int rate;
	void SetVolume(long volume);
	int Sync();
	int resets;
	void Clear();
	int max_freq;
	int min_freq;
	double slope;
	double get_requested_freq();
	void Reset();
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
	int GetMaxWrite(void);
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
