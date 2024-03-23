#include "sound.h"
#include <math.h>
#include <assert.h>
#include "config.h"
#include <algorithm>


extern c_config *config;

#define ReleaseCOM(x) { if(x) { x->Release(); x = 0; } }

//#define blah(x, ...) { if (x(__VA_ARGS__) == S_OK) { MessageBox(NULL, #x " failed", "Error", MB_OK); return 0;} }

c_sound::c_sound()
{
	resets = 0;
	requested_freq = freq = default_freq = 48000.0;
	max_freq = default_freq * 1.02;
	min_freq = default_freq * .98;
	adjustPeriod = 3;
	lastb = 0;
	ZeroMemory(&wf, sizeof(WAVEFORMATEX));
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 2;
	wf.nSamplesPerSec = 48000;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.wBitsPerSample / 8 * wf.nChannels;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	ema = 0.0;
	first_b = 1;
    slope = 0.0;
    value_index = 0;

	interleave_buffer = std::make_unique<uint32_t[]>(INTERLEAVE_BUFFER_LEN);
    buffer_wait_count = 0;
}

int c_sound::init_wasapi()
{
    HRESULT hr;

	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void **)&enumerator);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI CoCreateInstance failed", "Error", MB_OK);
        return 0;
    }

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI GetDefaultAudioEndpoint failed", "Error", MB_OK);
        return 0;
    }

    hr = device->Activate(IID_IAudioClient3, CLSCTX_ALL, NULL,(void **)&audio_client);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI Activate failed", "Error", MB_OK);
        return 0;
    }

	UINT32 pDefaultPeriodInFrames;
    UINT32 pFundamentalPeriodInFrames;
    UINT32 pMinPeriodInFrames;
    UINT32 pMaxPeriodInFrames;

	hr = audio_client->GetSharedModeEnginePeriod(&wf, &pDefaultPeriodInFrames,
                                            &pFundamentalPeriodInFrames , &pMinPeriodInFrames, &pMaxPeriodInFrames);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI GetSharedModeEnginePeriod failed", "Error", MB_OK);
        return 0;
    }

	//convert BUFFER_MSEC to length in 100ns
    const REFERENCE_TIME buffer_time = (REFERENCE_TIME)BUFFER_MSEC * 10000;
	
	hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, buffer_time, 0, &wf, NULL);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI Initialize failed", "Error", MB_OK);
        return 0;
    }

    hr = audio_client->GetBufferSize(&pNumBufferFrames);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI GetBufferSize failed", "Error", MB_OK);
        return 0;
    }
	
	hr = audio_client->GetService(IID_IAudioRenderClient, (void **)&render_client);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI GetService IAudioRenderClient failed", "Error", MB_OK);
        return 0;
    }

	hr = audio_client->GetService(IID_IAudioClock, (void **)&audio_clock);
    if (hr != S_OK) {
        MessageBox(NULL, "WASAPI GetService IAudioClock failed", "Error", MB_OK);
        return 0;
    }

    return 1;
}

c_sound::~c_sound()
{
    ReleaseCOM(audio_clock);
    ReleaseCOM(render_client);
    ReleaseCOM(audio_client);
    ReleaseCOM(device);
    ReleaseCOM(enumerator);
}

int c_sound::init()
{
    if (!init_wasapi()) {
        return 0;
    }

	reset();
	return 1;
}

void c_sound::play()
{
    reset_slope();
    BYTE *buffer;
    render_client->GetBuffer(pNumBufferFrames, &buffer);
    render_client->ReleaseBuffer(pNumBufferFrames, AUDCLNT_BUFFERFLAGS_SILENT);
    audio_client->Start();
    Sleep(50); //sleep for a bit to give the play cursor time to advance.  ick.
}

void c_sound::stop()
{
    audio_client->Stop();
    audio_client->Reset();
}

void c_sound::reset()
{
	freq = default_freq;
}

void c_sound::reset_slope()
{
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
    audio_clock->GetFrequency(&audio_frequency);
    audio_clock->GetPosition(&audio_position, NULL);
    audio_position_diff = audio_position - audio_last_position;
    audio_last_position = audio_position;

	const double alpha = 2.0 / (30 + 1);

	if (first_b) {
		ema = b;
		first_b = 0;
	}
	else {
		ema = (b - ema) * alpha + ema;
		b = (int)ema;
	}
	values[value_index] = b;
	value_index = (value_index + 1) % num_values;

    enum
    {
        CONVERGING = -1,
        STEADY = 0,
        DIVERGING = 1
    };
	
    if (--adjustPeriod == 0)
	{
		slope = calc_slope();
        double abs_slope = abs(slope);
		adjustPeriod = adjustFrames;
		int diff = b - target;
        int abs_diff = abs(diff);

        int trend = (diff * slope > 0.0) - (diff * slope < 0.0);

        if (trend == CONVERGING) {
            state = "converging";
        }
        else if (trend == DIVERGING) {
            state = "diverging";
        }
        else {
            state = "steady";
        }
		
		double new_adj = 0.0;

		if (false && b > (8000*2)) //near underflow
		{
			reset();
			resets++;
		}
		else if (trend == CONVERGING) //moving towards target at a abs(slope) > 1.0
		{
			//if we're more than 1000 bytes away from target, increase adjustment to cause us to converge faster
            //unless abs(slope) is above 3.0
            if (abs_diff > 1000) {
                if (abs_slope < 3.0) {
                    new_adj = -2.0;
                    //sprintf(buf, "fast converge diff: %d, slope: %f, fixed adj -2.0\n", abs_diff, abs_slope);
                    //OutputDebugString(buf);
                }
            }
            else if (abs_slope > 1.0) {
                //else if we're near the target and our abs(slope) is greater than one, back off on adjustment to reduce slope
                new_adj = abs_slope / 4.0;
                if (new_adj > 1.0)
                    new_adj = 1.0;
                //sprintf(buf, "near target diff: %d, slope: %f, adj: %f\n", abs_diff, abs_slope, new_adj);
                //OutputDebugString(buf);
            }
		}
		else if (trend == DIVERGING || b == 0) //moving away from target or stuck behind play cursor
		{
			//skew causes new_adj to increase faster when we're farther away from the target
			double skew = (abs_diff / (1600.0*2)) * 10.0;
			new_adj = -((abs_slope + skew) / 2.0);
			if (new_adj < -2.0)
				new_adj = -2.0;
            //sprintf(buf, "slope: %f, skew: %f, adj: %f\n", abs(slope), skew, new_adj);
            //OutputDebugString(buf);
		}
        int dir = (diff > 0) - (diff < 0);
		new_adj *= dir;
		freq += -new_adj;
        freq = std::clamp(freq, min_freq, max_freq);
		requested_freq = freq;
	}
    return b;
}

int c_sound::copy(const short *left, const short *right, int numSamples)
{
    int waited = 0;
	int src_len = numSamples * wf.nBlockAlign;

	uint32_t *ib = interleave_buffer.get();
    const uint16_t *l = (uint16_t*)left;
    const uint16_t *r = right != NULL ? (uint16_t*)right : (uint16_t*)left;
    for (int i = 0; i < numSamples; i++) {
        *ib++ = *l++ | (*r++ << 16);
    }
    while (get_max_write() < src_len) {
        waited = 1;
    }
    if (waited) {
        buffer_wait_count++;
    }
	BYTE *wasapi_buffer;
    render_client->GetBuffer(numSamples, &wasapi_buffer);
    uint32_t *wasapi_out = (uint32_t *)wasapi_buffer;
    ib = interleave_buffer.get();
    for (int i = 0; i < numSamples; i++) {
        *wasapi_out++ = *ib++;
    }
    render_client->ReleaseBuffer(numSamples, 0);
    return 0;	
}

int c_sound::get_max_write()
{
    UINT32 padding;
    audio_client->GetCurrentPadding(&padding);
    return (pNumBufferFrames - padding) * 4;
}

int c_sound::get_buffered_length()
{
    UINT32 padding;
    audio_client->GetCurrentPadding(&padding);
    return padding * 4;
}