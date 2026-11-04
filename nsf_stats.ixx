module;
#include "meow_fft.h"
#include "d3d11.h"
#include <DirectXMath.h>
export module nemulator:nsf_stats;
import nemulator.std;
import vec;
import shader_pass;

import system;
import :stats;
import dsp;

export class c_nsf_stats :
    public c_stats
{
public:
    c_nsf_stats();
    ~c_nsf_stats();
    void draw();
    void init(void* params);
    float x, y, z;
    int update(double dt, int child_result, void* params);
private:
    float aweight_filter(float sample);

    struct SimpleVertex
    {
        s_vec3 pos;
        s_vec2 tex;
    };
    c_shader_pass pass;
    s_transform_constants transform = {};
    ID3D11Buffer *transform_buffer = nullptr;
    ID3D11SamplerState *sampler = nullptr;
    ID3D11Texture2D* tex;
    ID3D11Buffer* vertex_buffer;
    ID3D11ShaderResourceView* tex_rv;
    ID3D11InputLayout* vertex_layout;


    c_system* c;

    static const int tex_size = 512;
    int draw_count;
    short sb[8192];
    short* sbp;
    int sb_index;
    int total_samples;
    float prev_x;
    float prev_y;
    int scale;
    double scale_timer;
    float max_level;
    double avg_level;

    static const int fft_length = 8192;

    static const int NUM_BANDS = 32;
    int fft_scale;
    float max_fft_level;
    double avg_fft_level;
    double bands[NUM_BANDS];
    double band_mins[NUM_BANDS];
    double band_maxs[NUM_BANDS];
    double bucket_vals[NUM_BANDS];
    int fft_rate;
    float in2[fft_length];
    float out2[fft_length];
    double bucket_update_timer;
    double mag_adjust;
    
        
    using bq1_t = dsp::c_biquad<1.0f, 0.197012037038803f, 0.394024074077606f, 0.197012037038803f,
        1.0f, -0.224558457732201f, 0.012606625445187f>;
    using bq2_t = dsp::c_biquad<1.0f, 1.0f, -2.0f, 1.0f, 1.0f, -1.893870472908020f, 0.895159780979157f>;
    using bq3_t = dsp::c_biquad<1.0f, 1.0f, -2.0f, 1.0f, 1.0f, -1.994614481925964f, 0.994621694087982f>;

    std::unique_ptr<bq1_t> biquad1;
    std::unique_ptr<bq2_t> biquad2;
    std::unique_ptr<bq3_t> biquad3;
    double scroll_timer;
    double scroll_offset;
    Meow_FFT_Complex* meow_out;
    Meow_FFT_Workset_Real* fft_real;

    int visualization;

    enum VISUALIZATIONS {
        VIZ_FFT,
        VIZ_SCOPE,
        VIZ_BOTH,
        NUM_VISUALIZATIONS
    };
};