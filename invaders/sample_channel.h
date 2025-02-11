#pragma once
#include <memory>

namespace invaders
{
class c_sample_channel
{
  public:
    c_sample_channel();
    ~c_sample_channel() {};
    int loop;
    void trigger();
    void stop();
    float get_sample();
    void load_wav(char *buf);
    int is_active();
    void set_playback_rate(double playback_freq);
    void reset();

  private:
    int len;
    int freq;
    int active;
    double step;
    double pos;
    std::unique_ptr<int16_t[]> data;
};
} //namespace invaders