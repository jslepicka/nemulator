#pragma once
#include <Audioclient.h>
#include <cstdint>
#include <mmdeviceapi.h>
#include <string>

class c_sound
{
  public:
    c_sound();
    ~c_sound();
    int init();
    void play();
    void stop();
    int copy(const short *left, const short *right, int numSamples);
    double get_freq()
    {
        return freq;
    }
    int get_max_freq()
    {
        return max_freq;
    }
    int rate;
    int sync();
    int resets;
    double max_freq;
    double min_freq;
    double slope;
    double get_requested_freq();
    void reset();
    int get_buffered_length();
    uint32_t buffer_wait_count;
    int init_wasapi();

    UINT64 audio_frequency = 0;
    UINT64 audio_position = 0;
    UINT64 audio_last_position = 0;
    UINT64 audio_position_diff = 0;

    std::string state = "";

  private:
    double requested_freq;
    double default_freq;

    double freq;
    int adjustPeriod;
    int lastb;
    static const int BUFFER_MSEC = 80;
    static const int adjustFrames = 3;
    static const int target = 48000 * 2 / 60 * 1 * 2;//1600*3;
    WAVEFORMATEX wf;
    int get_max_write();
    double calc_slope();
    void reset_slope();

    static const int num_values = 60;
    int values[num_values];
    int freq_values[num_values];
    int value_index;

    double ema;
    int first_b;

    uint32_t *interleave_buffer;
    const int INTERLEAVE_BUFFER_LEN = 1024;

    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
    const IID IID_IAudioClient3 = __uuidof(IAudioClient3);
    const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
    const IID IID_IAudioClock = __uuidof(IAudioClock);

    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioClient3 *audio_client = NULL;
    IAudioRenderClient *render_client = NULL;
    IAudioClock *audio_clock = NULL;
    UINT32 pNumBufferFrames = 0;
};
