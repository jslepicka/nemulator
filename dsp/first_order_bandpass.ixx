module;

export module dsp:first_order_bandpass;
import nemulator.std;
import :audio_filter;

namespace dsp
{
export class c_first_order_bandpass : public i_audio_filter
{
  public:
    // default to 2Hz-12kHz
    // Matlab:
    // fs = 48000;
    // fc_l = 12000;
    // fc_h = 2;
    // [ b_l, a_l ] = butter(1, fc_l / (fs / 2), 'low');
    // [ b_h, a_h ] = butter(1, fc_h / (fs / 2), 'high');
    // b0_l = num2str(b_l(1), '%.16ff');
    // b1_l = num2str(b_l(2), '%.16ff');
    // a1_l = num2str(a_l(2), '%.16ff');
    // b0_h = num2str(b_h(1), '%.16ff');
    // b1_h = num2str(b_h(2), '%.16ff');
    // a1_h = num2str(a_h(2), '%.16ff');
    // fprintf('%s, %s, %s, %s, %s, %s\n', b0_l, b1_l, a1_l, b0_h, b1_h, a1_h);
    c_first_order_bandpass() :
        b0_l(0.5000000000000000f),
        b1_l(0.5000000000000000f),
        a1_l(-0.0000000000000001f),
        b0_h(0.9998691174378402f),
        b1_h(-0.9998691174378402f),
        a1_h(-0.9997382348756805f),
        w_l(0.0f),
        w_h(0.0f)
    {};
    
    c_first_order_bandpass(float b0_l, float b1_l, float a1_l, float b0_h, float b1_h, float a1_h)
    {
        this->b0_l = b0_l;
        this->b1_l = b1_l;
        this->a1_l = a1_l;
        this->b0_h = b0_h;
        this->b1_h = b1_h;
        this->a1_h = a1_h;
        w_l = 0.0f;
        w_h = 0.0f;
    };

    __forceinline float process(float in)
    {
        //direct form ii
        float w_l_prev = w_l;
        w_l = in - a1_l * w_l_prev;
        float out_l = b0_l * w_l + b1_l * w_l_prev;
        
        float w_h_prev = w_h;
        w_h = out_l - a1_h * w_h_prev;
        return b0_h * w_h + b1_h * w_h_prev;
    }

  private:
    float w_l;
    float w_h;
    float b0_l;
    float b1_l;
    float a1_l;
    float b0_h;
    float b1_h;
    float a1_h;
};
} //namespace dsp