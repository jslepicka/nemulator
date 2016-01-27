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

#include "sound.h"
#include <math.h>
#include <assert.h>
#include "config.h"
//#include <fstream>
//
//std::ofstream file;
extern c_config *config;

Sound::Sound(HWND hWnd)
{

	this->hWnd = hWnd;
	resets = 0;
	requested_freq = freq = default_freq = 48000;
	max_freq = default_max_freq = (int)(default_freq * 1.02);
	min_freq = default_min_freq = (int)(default_freq * .98);
	lpDS = NULL;
	adjustPeriod = 3;
	lastb = 0;
	ZeroMemory(&wf, sizeof(WAVEFORMATEX));
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 1;
	wf.nSamplesPerSec = 48000;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.wBitsPerSample / 8 * wf.nChannels;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	ZeroMemory(&bufferdesc, sizeof(DSBUFFERDESC));
	bufferdesc.dwSize = sizeof(DSBUFFERDESC); 
	bufferdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_TRUEPLAYPOSITION | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME; 
	bufferdesc.dwBufferBytes = (wf.nAvgBytesPerSec * BUFFER_MSEC)/1000;
	bufferdesc.lpwfxFormat = &wf;

	ZeroMemory(&primaryBufferDesc, sizeof(DSBUFFERDESC));
	primaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	primaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	primaryBufferDesc.dwBufferBytes = 0;

	write_cursor = 0;
}

Sound::~Sound(void)
{
	if (buffer)
	{
		Stop();
		buffer->Release();
		buffer = NULL;
	}

	if (lpDS)
	{
		lpDS->Release();
		lpDS = NULL;
	}
}

int Sound::Init(void)
{
	if (DS_OK != DirectSoundCreate(NULL, &lpDS, NULL))
	{
		MessageBox(NULL, "DirectSoundCreate failed", "", MB_OK);
		return 0;
	}
	if (DS_OK != IDirectSound_SetCooperativeLevel(lpDS, hWnd, DSSCL_PRIORITY))
	{
		MessageBox(NULL, "IDirectSound_SetCooperativeLevel failed", "", MB_OK);
		return 0;
	}
	if (DS_OK != lpDS->CreateSoundBuffer(&primaryBufferDesc, &primaryBuffer, NULL))
	{
		MessageBox(NULL, "Unable to create primary sound buffer", "", MB_OK);
		return 0;
	}
	if (DS_OK != primaryBuffer->SetFormat(&wf))
	{
		MessageBox(NULL, "Unable to set primary buffer format", "", MB_OK);
		return 0;
	}

	primaryBuffer->Release();
	primaryBuffer = NULL;
	if (DS_OK != lpDS->CreateSoundBuffer(&bufferdesc, &buffer, NULL))
	{
		MessageBox(NULL, "CreateSoundBuffer failed", "", MB_OK);
		return 0;
	}
	Clear();
	Reset();
	return 1;
}

void Sound::Play(void)
{
	buffer->Play(0, 0, DSBPLAY_LOOPING);
}

void Sound::Stop(void)
{
	buffer->Stop();
}

void Sound::SetVolume(long volume)
{
	buffer->SetVolume(volume);
}

void Sound::Reset()
{
	buffer->SetCurrentPosition((write_cursor + target/* + 3200*/) % bufferdesc.dwBufferBytes );
	freq = default_freq;
	max_freq = default_max_freq;
	min_freq = default_min_freq;

	slope = 0.0;
	value_index = 0;
	for (int i = 0; i < num_values; i++)
		values[i] = -1;
}

double Sound::get_requested_freq()
{
	return requested_freq;
}

int Sound::Sync()
{
	int b = GetMaxWrite();

	values[value_index] = b;
	value_index = (value_index + 1) % num_values;
	int valid_values = 0;
	int sx = 0;
	int sy = 0;
	int sxx = 0;
	int sxy = 0;
	for (int i = value_index, j = 0; j < num_values; j++)
	{
		if (values[i] != -1)
		{
			int k = valid_values;
			sx += k;
			sy += values[i];
			sxx += (k*k);
			sxy += (k*values[i]);
			valid_values++;
		}
		i = (i + 1) % num_values;
	}
	double num = (double)(valid_values * sxy - sx*sy);
	double den = (double)(valid_values * sxx - sx*sx);
	slope = den == 0.0 ? 0.0 : num/den;
	
	if (--adjustPeriod == 0)
	{
		adjustPeriod = adjustFrames;
		int diff = b - target;
		
		//dir = 1 if diff > 0, -1 if diff < 0, else 0
		int dir = (diff > 0) - (diff < 0);

		double new_adj = 0.0;

		if (b > 8000) //near underflow
		{
			Reset();
			resets++;
		}
		else if (dir * slope < -1.0) //moving towards target at a slope > 1.0
		{
			new_adj = abs(slope)/4.0;
			if (new_adj > 1.0)
				new_adj = 1.0;
		}
		else if (dir * slope > 0.0 || b == 0) //moving away from target or stuck behind play cursor
		{
			//skew causes new_adj to increase faster when we're farther away from the target
			double skew = (abs(diff) / 1600.0) * 10.0;
			new_adj = -((abs(slope) + skew) / 2.0);
			if (new_adj < -2.0)
				new_adj = -2.0;
		}
		new_adj *= dir;
		freq += -new_adj;
		if (freq < min_freq)
			freq = min_freq;
		if (freq > max_freq)
			freq = max_freq;
		requested_freq = freq;
	}
	return b;
}

void Sound::Clear()
{
	LPBYTE lpbuf1 = NULL;
	LPBYTE lpbuf2 = NULL;
	DWORD dwsize1;

	buffer->Lock(0, 0, (LPVOID*)&lpbuf1, &dwsize1, NULL, NULL, DSBLOCK_ENTIREBUFFER);
	for (int i = 0; i < (int)dwsize1; i++)
	{
		lpbuf1[i] = 0;
	}
	buffer->Unlock(lpbuf1, dwsize1, NULL, NULL);
}

int Sound::Copy(const short *src, int numSamples)
{
	HRESULT hr;
	LPBYTE lpbuf1 = NULL;
	LPBYTE lpbuf2 = NULL;
	DWORD dwsize1 = 0;
	DWORD dwsize2 = 0;
	DWORD dwbyteswritten1 = 0;
	DWORD dwbyteswritten2 = 0;
	int src_len = numSamples * wf.nBlockAlign;
	//wait until there's enough room in the DirectSound buffer for the new samples
	while (GetMaxWrite() < src_len);

	// Lock the sound buffer
	hr = buffer->Lock(write_cursor, (DWORD)src_len, (LPVOID *)&lpbuf1, &dwsize1, (LPVOID *)&lpbuf2, &dwsize2, 0);
	if (hr == DS_OK)
	{
		memcpy(lpbuf1, src, dwsize1);
		dwbyteswritten1 = dwsize1;
		if (lpbuf2)
		{
			dwbyteswritten2 = src_len - dwsize1;
			memcpy(lpbuf2, src + (dwsize1 / wf.nBlockAlign), dwbyteswritten2);
		}

		// Update our buffer offset and unlock sound buffer
		write_cursor = (write_cursor + src_len) % bufferdesc.dwBufferBytes;
		buffer->Unlock(lpbuf1, dwbyteswritten1, lpbuf2, dwbyteswritten2);
	}
	return 0;		
}

int Sound::GetMaxWrite(void)
{
	DWORD playCursor;
	buffer->GetCurrentPosition(&playCursor, NULL);

	if (write_cursor <= playCursor)
		return playCursor - write_cursor;
	else
		return bufferdesc.dwBufferBytes - write_cursor + playCursor;
}