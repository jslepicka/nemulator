module;
#include "meow_fft.h"
#include "d3d10.h"
#include "d3dx10.h"
export module nemulator:nsf_stats;
import nemulator.std;

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
        D3DXVECTOR3 pos;
        D3DXVECTOR2 tex;
    };
    ID3D10Effect* effect;
    ID3D10Texture2D* tex;
    ID3D10EffectTechnique* technique;
    ID3D10EffectShaderResourceVariable* var_tex;
    ID3D10Buffer* vertex_buffer;
    ID3D10ShaderResourceView* tex_rv;
    ID3D10InputLayout* vertex_layout;
    D3DXMATRIX matrix_world;

    ID3D10EffectMatrixVariable* var_world;
    ID3D10EffectMatrixVariable* var_view;
    ID3D10EffectMatrixVariable* var_proj;

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
    //c_biquad* biquad1;
    //c_biquad* biquad2;
    //c_biquad* biquad3;
    std::unique_ptr<dsp::c_biquad> biquad1;
    std::unique_ptr<dsp::c_biquad> biquad2;
    std::unique_ptr<dsp::c_biquad> biquad3;
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