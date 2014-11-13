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
//#include <fstream>
//
//std::ofstream file;

Sound::Sound(HWND hWnd)
{

	this->hWnd = hWnd;
	resets = 0;
	//freq = default_freq = 48300;
	requested_freq = freq = default_freq = 48000;//44671;
	max_freq = default_max_freq = (int)(default_freq * 1.02);
	min_freq = default_min_freq = (int)(default_freq * .98);
	lpDS = NULL;
	adjustPeriod = 3;
	lastb = 0;
	ZeroMemory(&wf, sizeof(WAVEFORMATEX));
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 2;
	wf.nSamplesPerSec = 48000;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.wBitsPerSample / 8 * wf.nChannels;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	ZeroMemory(&bufferdesc, sizeof(DSBUFFERDESC));
	bufferdesc.dwSize = sizeof(DSBUFFERDESC); 
	//bufferdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME; 
	bufferdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_TRUEPLAYPOSITION | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME; 
	bufferdesc.dwBufferBytes = (wf.nAvgBytesPerSec * BUFFER_MSEC)/1000;
	bufferdesc.lpwfxFormat = &wf;

	ZeroMemory(&primaryBufferDesc, sizeof(DSBUFFERDESC));
	primaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	primaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	primaryBufferDesc.dwBufferBytes = 0;

	bufferOffset = 0;

	//delay_len = wf.nSamplesPerSec * (5.0 / 1000.0);
	delay_len = 256;

	delay_line = new short[delay_len];
	for (int i = 0; i < delay_len; i++)
		delay_line[i] = 0;
	delay_write = delay_len - 1;
	delay_read = 0;
}

Sound::~Sound(void)
{
	delete[] delay_line;
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
		//MessageBox(NULL, "CreateSoundBuffer failed", "", MB_OK);
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
	buffer->SetCurrentPosition((bufferOffset + target/* + 3200*/) % bufferdesc.dwBufferBytes );
	//buffer->SetCurrentPosition(bufferOffset);
	//DWORD play_pos, write_pos;
	//buffer->GetCurrentPosition(&play_pos, &write_pos);
	//buffer->SetCurrentPosition((write_pos - target) % bufferdesc.dwBufferBytes);


	//resets++;
	freq = default_freq;
	//buffer->SetFrequency(freq);
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

		if (b > 8000)
		{
			Reset();
			resets++;
		}
		else if (dir * slope < -1.0)
		{
			new_adj = abs(slope)/4.0;
			if (new_adj > 1.0)
				new_adj = 1.0;
		}
		else if (dir * slope > 0.0 || b == 0)
		{
				double skew = (abs(diff)/1600.0) * 10.0;
				new_adj = -((abs(slope)+skew)/2.0);
				if (new_adj < -2.0)
					new_adj = -2.0;
		}
		new_adj *= dir;
		freq += -new_adj;
		if (freq < min_freq)
			freq = min_freq;
		if (freq > max_freq)
			freq = max_freq;
		//buffer->SetFrequency(freq);
		requested_freq = freq;
	}
	return b;
}

void Sound::SetMaxFps(double fps)
{
	max_freq = (int)(fps / 60.0 * 48000 * 1.01);
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
	//wait until there's enough room in the DirectSound buffer for the new samples
	while ((GetMaxWrite() & ~1) < wf.nChannels * numSamples * 2);

	// Lock the sound buffer
	hr = buffer->Lock(bufferOffset, (DWORD)(wf.nChannels * numSamples * 2), (LPVOID *)&lpbuf1, &dwsize1, (LPVOID *)&lpbuf2, &dwsize2, 0);
	if (hr == DS_OK)
	{
		if ((dwbyteswritten1 = CopyBuffer(lpbuf1, dwsize1, src, numSamples)) == dwsize1)
		{
			if (lpbuf2)
			{
				dwbyteswritten2 = CopyBuffer(lpbuf2, dwsize2, src + dwbyteswritten1 / 2 / wf.nChannels, numSamples - dwbyteswritten1 / 2 / wf.nChannels);
			}
			// Update our buffer offset and unlock sound buffer
			bufferOffset = (bufferOffset + dwbyteswritten1 + dwbyteswritten2) % bufferdesc.dwBufferBytes;
			buffer->Unlock (lpbuf1, dwbyteswritten1, lpbuf2, dwbyteswritten2);
		}
	}
	return 0;		
}

int Sound::CopyBuffer(LPBYTE destBuffer, DWORD destBufferSize, const short *src, int srcCount)
{
	int copy_size = 0;
	destBufferSize &= ~1;
	//if (destBufferSize >= srcCount * 2 * wf.nChannels)
	//	copy_size = srcCount * 2 * wf.nChannels;
	//else
	//	copy_size = (destBufferSize & ~1);
	//memcpy(destBuffer, src, copy_size);
	copy_size = 0;
	short *s = (short*)src;
	short *d = (short*)destBuffer;
	for (int i = 0; i < ((destBufferSize >= srcCount * 2 * wf.nChannels) ? srcCount : (destBufferSize / 2 / wf.nChannels)); i++)
	{
		if (wf.nChannels == 1)
		{
			*d++ = *s++;
			copy_size += 2;
		}
		else if (wf.nChannels == 2)
		{
			*d++ = *s;
			*d++ = delay_line[delay_read];
			delay_line[delay_write] = *s;
			s++;
			//delay_read = (delay_read + 1) % delay_len;
			//delay_write = (delay_write + 1) % delay_len;
			delay_read = (delay_read + 1) & 0xFF;
			delay_write = (delay_write + 1) & 0xFF;
			copy_size += 4;

		}
	}
	return copy_size;
}

int Sound::GetMaxWrite(void)
{
	DWORD playCursor;
	buffer->GetCurrentPosition(&playCursor, NULL);

	if (bufferOffset <= playCursor)
		return playCursor - bufferOffset;
	else
		return bufferdesc.dwBufferBytes - bufferOffset + playCursor;
}