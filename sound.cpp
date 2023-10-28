#include "sound.h"
#include <math.h>
#include <assert.h>
#include "config.h"

extern c_config *config;

c_sound::c_sound(HWND hWnd)
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
	wf.nChannels = 2;
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

	ema = 0.0;
	first_b = 1;

	interleave_buffer = new uint32_t[INTERLEAVE_BUFFER_LEN];
}

c_sound::~c_sound()
{
	if (buffer)
	{
		stop();
		buffer->Release();
		buffer = NULL;
	}

	if (lpDS)
	{
		lpDS->Release();
		lpDS = NULL;
	}

	if (interleave_buffer) {
        delete[] interleave_buffer;
	}
}

int c_sound::init()
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
	clear();
	reset();
	return 1;
}

void c_sound::play()
{
	buffer->Play(0, 0, DSBPLAY_LOOPING);
}

void c_sound::stop()
{
	buffer->Stop();
}

void c_sound::set_volume(long volume)
{
	buffer->SetVolume(volume);
}

void c_sound::reset()
{
	buffer->SetCurrentPosition((write_cursor + target/* + 3200*/) % bufferdesc.dwBufferBytes );
	freq = default_freq;
	max_freq = default_max_freq;
	min_freq = default_min_freq;

	slope = 0.0;
	value_index = 0;
	for (int i = 0; i < num_values; i++)
		values[i] = -1;

	ema = 0.0;
	first_b = 1;
}

double c_sound::get_requested_freq()
{
	return requested_freq;
}

double c_sound::calc_slope()
{
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
	return den == 0.0 ? 0.0 : num / den / 2.0;
}

int c_sound::sync()
{
	int b = get_max_write();

	const double alpha = 2.0 / (15 + 1);

	if (first_b) {
		ema = b;
		first_b = 0;
	}
	else {
		ema = (b - ema) * alpha + ema;
		b = ema;
	}
	values[value_index] = b;
	value_index = (value_index + 1) % num_values;
	
	if (--adjustPeriod == 0)
	{
		slope = calc_slope();
		adjustPeriod = adjustFrames;
		int diff = b - target;
		
		//dir = 1 if diff > 0, -1 if diff < 0, else 0
		int dir = (diff > 0) - (diff < 0);

		double new_adj = 0.0;

		if (b > (8000*2)) //near underflow
		{
			reset();
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
			double skew = (abs(diff) / (1600.0*2)) * 10.0;
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

void c_sound::clear()
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

int c_sound::copy(const short *left, const short *right, int numSamples)
{
	HRESULT hr;
	LPBYTE lpbuf1 = NULL;
	LPBYTE lpbuf2 = NULL;
	DWORD dwsize1 = 0;
	DWORD dwsize2 = 0;
	DWORD dwbyteswritten1 = 0;
	DWORD dwbyteswritten2 = 0;
	int src_len = numSamples * wf.nBlockAlign;
	
	uint32_t *ib = interleave_buffer;
    const uint16_t *l = (uint16_t*)left;
    const uint16_t *r = right != NULL ? (uint16_t*)right : (uint16_t*)left;
    for (int i = 0; i < numSamples; i++) {
        *ib++ = *l++ | (*r++ << 16);
    }
	
	//wait until there's enough room in the DirectSound buffer for the new samples
	while (get_max_write() < src_len);

	// Lock the sound buffer
	hr = buffer->Lock(write_cursor, (DWORD)src_len, (LPVOID *)&lpbuf1, &dwsize1, (LPVOID *)&lpbuf2, &dwsize2, 0);
	if (hr == DS_OK)
	{
		memcpy(lpbuf1, interleave_buffer, dwsize1);
		dwbyteswritten1 = dwsize1;
		if (lpbuf2)
		{
			dwbyteswritten2 = src_len - dwsize1;
			memcpy(lpbuf2, interleave_buffer + (dwsize1 / wf.nBlockAlign), dwbyteswritten2);
		}

		// Update our buffer offset and unlock sound buffer
		write_cursor = (write_cursor + src_len) % bufferdesc.dwBufferBytes;
		buffer->Unlock(lpbuf1, dwbyteswritten1, lpbuf2, dwbyteswritten2);
	}
    else {
        int x = 1;
    }
	return 0;		
}

int c_sound::get_max_write()
{
	DWORD playCursor;
	buffer->GetCurrentPosition(&playCursor, NULL);

	if (write_cursor <= playCursor)
		return playCursor - write_cursor;
	else
		return bufferdesc.dwBufferBytes - write_cursor + playCursor;
}

int c_sound::get_buffered_length()
{
    DWORD play_cursor;
    buffer->GetCurrentPosition(&play_cursor, NULL);
    if (write_cursor >= play_cursor) {
        return write_cursor - play_cursor;
    }
    else {
        return bufferdesc.dwBufferBytes - play_cursor + write_cursor;
    }
}