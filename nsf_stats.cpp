module;
#define MEOW_FFT_IMPLEMENTATION
#include "meow_fft.h"
#include "d3d10.h"
#include "D3DX10.h"
#include "effect2.fxo.h"

module nemulator:nsf_stats;

import input_handler;

extern ID3D10Device* d3dDev;
extern D3DXMATRIX matrixView;
extern D3DXMATRIX matrixProj;
extern std::unique_ptr<c_input_handler> g_ih;

import nemulator.buttons;

c_nsf_stats::c_nsf_stats()
{
    meow_out = NULL;
    fft_real = NULL;
    visualization = 0;
}

c_nsf_stats::~c_nsf_stats()
{
    if (meow_out) {
        free(meow_out);
    }
    if (fft_real) {
        free(fft_real);
    }
}

void c_nsf_stats::init(void* params)
{
    draw_count = 0;
    sbp = sb;
    this->c = (c_system*)params;
    c_stats::init(NULL);


    HRESULT hr;
    D3D10_INPUT_ELEMENT_DESC elementDesc[] =
    {
        { TEXT("POSITION"), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { TEXT("TEXCOORD"), 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof(elementDesc) / sizeof(elementDesc[0]);
    hr = D3DX10CreateEffectFromMemory((LPCVOID)effect2, sizeof(effect2), "effect1", NULL, NULL, "fx_4_0", 0, 0, d3dDev, NULL, NULL, &effect, NULL, NULL);
    if (FAILED(hr))
    {
        MessageBox(NULL, "D3DX10CreateEffectFromFile failed", 0, 0);
        PostQuitMessage(0);
        return;
    }
    technique = effect->GetTechniqueByName("Render");
    var_tex = effect->GetVariableByName("txDiffuse")->AsShaderResource();
    var_world = effect->GetVariableByName("World")->AsMatrix();
    var_proj = effect->GetVariableByName("Projection")->AsMatrix();
    var_view = effect->GetVariableByName("View")->AsMatrix();

    D3D10_PASS_DESC passDesc;
    technique->GetPassByIndex(0)->GetDesc(&passDesc);

    hr = d3dDev->CreateInputLayout(elementDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertex_layout);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateInputLayout failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    SimpleVertex vertices[] =
    {
        { D3DXVECTOR3(-.75f, -.5f, 0.0f), D3DXVECTOR2(0.0f, 1.0f) },
        { D3DXVECTOR3(-.75f, -.1f, 0.0f), D3DXVECTOR2(0.0f, 0.0f) },
        { D3DXVECTOR3(.75f, -.5f, 0.0f), D3DXVECTOR2(1.0f, 1.0f) },
        { D3DXVECTOR3(.75f, -.1f, 0.0f), D3DXVECTOR2(1.0f, 0.0f) },
    };

    D3D10_BUFFER_DESC bd;
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 4;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem = vertices;
    hr = d3dDev->CreateBuffer(&bd, &initData, &vertex_buffer);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateBuffer failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3D10_TEXTURE2D_DESC texDesc;
    texDesc.Width = tex_size;
    texDesc.Height = tex_size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D10_USAGE_DYNAMIC;
    //texDesc.Usage = D3D10_USAGE_DEFAULT;
    texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    //texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    hr = d3dDev->CreateTexture2D(&texDesc, 0, &tex);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateTexture2D failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3D10_SHADER_RESOURCE_VIEW_DESC descRv;
    descRv.Format = texDesc.Format;
    descRv.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    descRv.Texture2D.MipLevels = 1;
    descRv.Texture2D.MostDetailedMip = 0;
    hr = d3dDev->CreateShaderResourceView(tex, &descRv, &tex_rv);
    if (FAILED(hr))
    {
        MessageBox(NULL, "CreateShaderResourceView failed", 0, 0);
        PostQuitMessage(0);
        return;
    }

    D3DXMatrixIdentity(&matrix_world);
    var_tex->SetResource(tex_rv);
    prev_x = 0.0;
    prev_y = 0.0;
    scale = 1;
    scale_timer = 0.0;
    bucket_update_timer = 0.0;
    max_level = 0.0f;
    avg_level = 0;

    max_fft_level = 0.0f;
    avg_fft_level = 0.0f;
    fft_scale = 1;


    double f = 15.625;
    for (int i = 0; i < NUM_BANDS; i++)
    {
        bands[i] = f;
        band_mins[i] = f / pow(2.0, 1.0 / 6.0);
        band_maxs[i] = f * pow(2.0, 1.0 / 6.0);
        f *= pow(2.0, 1.0 / 3.0);

    }

    mag_adjust = 20.0 * log10(fft_length * 32768);//168.57679757; //20 * log10(8192.0*32768)
    for (auto& b : bucket_vals) {
        b = 0.0;
    }

    //a weighting filter
    biquad1 = std::make_unique<dsp::c_biquad>(1.0f,
        std::array{ 0.197012037038803f, 0.394024074077606f, 0.197012037038803f },
        std::array{ 1.0f, -0.224558457732201f, 0.012606625445187f });
    biquad2 = std::make_unique<dsp::c_biquad>(1.0f,
        std::array{ 1.0f, -2.0f, 1.0f },
        std::array{ 1.0f, -1.893870472908020f, 0.895159780979157f });
    biquad3 = std::make_unique<dsp::c_biquad>(1.0f,
        std::array{ 1.0f, -2.0f, 1.0f },
        std::array{ 1.0f, -1.994614481925964f, 0.994621694087982f });

    scroll_offset = 1.5;
    scroll_timer = 0.0;
    meow_out = (Meow_FFT_Complex*)malloc(sizeof(Meow_FFT_Complex) * fft_length);
    size_t workset_bytes = meow_fft_generate_workset_real(fft_length, NULL);
    fft_real = (Meow_FFT_Workset_Real*)malloc(workset_bytes);
    meow_fft_generate_workset_real(fft_length, fft_real);
}

int c_nsf_stats::update(double dt, int child_result, void* params)
{
    scale_timer += dt;

    if (scale_timer >= 100.0) {
        scale_timer -= 100.0;
        if (max_level < 1.0 && avg_level < .5) {
            scale++;
            if (scale > 25) {
                scale = 25;
            }
        }
        else if (scale > 1 && max_level >= 1.0) {
            int reduce = (int)round(max_level);
            scale -= reduce;
            if (scale < 1) {
                scale = 1;
            }
        }

        max_level = 0;
        max_fft_level = 0.0;
    }

    bucket_update_timer += dt;
    if (bucket_update_timer >= 10.0) {
        for (int i = 0; i < NUM_BANDS; i++) {
            double v = bucket_vals[i];
            if (v > 0.0) {
                v -= .05;
                if (v < 0.0) {
                    v = 0.0;
                }
            }
            bucket_vals[i] = v;
        }
        bucket_update_timer = 0.0;
    }

    scroll_timer += dt;
    if (scroll_timer >= 10.0) {
        scroll_offset -= .075;
        if (scroll_offset < 0.0) {
            scroll_offset = 0.0;
        }
        scroll_timer = 0.0;
    }

    if (g_ih->get_result(BUTTON_1A, true) & c_input_handler::RESULT_DOWN) {
        visualization = (visualization + 1) % NUM_VISUALIZATIONS;
    }
    else if (g_ih->get_result(BUTTON_1B, true) & c_input_handler::RESULT_DOWN) {
        if (--visualization < 0) {
            visualization = NUM_VISUALIZATIONS - 1;
        }
    }

    return c_stats::update(dt, child_result, params);
}

float c_nsf_stats::aweight_filter(float sample)
{
    return (float)biquad3->process(biquad2->process(biquad1->process(sample))) * 1.189276456832886f;
}


void c_nsf_stats::draw()
{
    draw_count++;
    const short* buf_l;
    const short* buf_r;

    int num_samples = c->get_sound_bufs(&buf_l, &buf_r);

    for (int i = 0; i < num_samples; i++) {
        //*sbp++ = *buf_l++;
        sb[(sb_index + i) & 8191] = *buf_l++;
    }

    sb_index = (sb_index + num_samples) & 8191;

    int ss = sb_index;
    float* op = in2;
    const char* err_str = NULL;

    for (int i = 0; i < fft_length; i++) {
        short sample = sb[ss];
        float filtered = aweight_filter(sample);
        *op = (short)filtered;
        ss = (ss + 1) & 8191;
        op++;
    }

    D3D10_MAPPED_TEXTURE2D map;
    tex->Map(0, D3D10_MAP_WRITE_DISCARD, NULL, &map);
    int* p = (int*)map.pData;
    int* pp = p;
    for (int i = 0; i < tex_size * tex_size; i++)
    {
        *pp++ = 0xF0202020;
    }

    auto plot = [&](int x, int y, int c) {
        if (x < 0 || x >= tex_size || y < 0 || y >= tex_size) {
            return;
        }
        *(p + y * tex_size + x) = c;
    };

    if (visualization == VIZ_FFT || visualization == VIZ_BOTH) {
        meow_fft_real(fft_real, in2, meow_out);

        const double db_max = 100.0;
        for (int bucket = 0; bucket < NUM_BANDS; bucket++) {
            double band_min = band_mins[bucket];
            double band_max = band_maxs[bucket];
            double f = 24000.0 / (double)(fft_length / 2.0);
            int start = (int)(band_min / f);
            int end = (int)(band_max / f);
            double avg = 0.0;
            int count = 0;
            double max = 0.0;
            for (int i = start; i <= end; i++) {
                float re = meow_out[i].r;
                //float im = meow_out[i + fft_length / 2].j;
                float im = meow_out[i].j;
                //re /= fft_length;
                //im /= fft_length;
                double mag = sqrt(re * re + im * im);
                if (mag > 0.0) {
                    mag = 20.0 * log10(mag);
                    mag -= mag_adjust;
                    mag += 130.0;
                    mag *= fft_scale;
                }
                else {
                    mag = 0.0;
                }
                if (mag > max)
                {
                    max = mag;
                }
            }
            double v = max / db_max;
            if (v > max_fft_level)
            {
                max_fft_level = (float)v;
            }

            if (v > bucket_vals[bucket]) {
                bucket_vals[bucket] = v;
            }
        }

        sbp = sb;
        draw_count = 0;

        int last_bucket = -1;
        auto lerp = [](auto start, auto end, double mu) {
            mu = std::clamp(mu, 0.0, 1.0);
            return start * (1.0 - mu) + end * mu;
        };

        for (int i = 0; i < tex_size; i++)
        {
            int bucket = (int)(i / (double)tex_size * (double)NUM_BANDS);
            double lower = .5;
            int py = (int)(((bucket_vals[bucket] - lower) * (1.0 / (1.0 - lower))) * (tex_size - 1));
            py = tex_size - 1 - py;
            if (py < 0) {
                py = 0;
            }
            int last_pixel_in_bucket = 0;

            if ((int)((i + 1) / (double)tex_size * (double)NUM_BANDS) != bucket) {
                last_pixel_in_bucket = 1;
            }
            unsigned int colors[] = { 0, 0xFF000000 };
            for (int y = py; y < tex_size; y++)
            {
                double mu = (tex_size - y) / (double)tex_size;
                int r = (int)lerp(0x10, 0xFF, mu * 1.25);
                int g = (int)lerp(0x28, 0xFF, mu * 1.25);
                int b = (int)lerp(0x80, 0xFF, mu);
                colors[0] = 0xFF000000 | ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);

                int color_select = (y == py || bucket != last_bucket || last_pixel_in_bucket);
                plot(i, y, colors[color_select]);
            }
            last_bucket = bucket;
        }

    }

    if (visualization == VIZ_SCOPE || visualization == VIZ_BOTH) {
        int mult = 4;

        int steps = 8192 / tex_size / mult;

        for (int i = 0; i < tex_size / mult; i++) {
            //int vol = *(sb + (i * steps));
            int vol = sb[(sb_index + (i * steps)) & 8191];
            vol *= scale;
            vol += 32768; //move sample level to 0 -> 65535
            float yy = vol / 65535.0f;
            float new_level = abs(yy * 2.0f - 1.0f);
            yy = std::clamp(yy, 0.0f, 1.0f);


            if (new_level > max_level)
            {
                max_level = new_level;
            }

            int c = 0xFFFFFFFF;
            int cur_x = (i + 1) * mult;
            if (prev_x != -1.0) {
                float dx = (float)cur_x - prev_x;
                float dy = yy - prev_y;

                for (int j = (int)prev_x; j < cur_x; j++) {
                    float yyy = prev_y + dy * (float)(j - prev_x) / dx;
                    int px = j;
                    int py = (int)round((yyy * (float)(tex_size - 1)));
                    plot(px, py, c);

                    plot(px - 1, py, c);
                    plot(px + 1, py, c);

                    plot(px - 1, py - 1, c);
                    plot(px, py - 1, c);
                    plot(px + 1, py - 1, c);

                    plot(px - 1, py + 1, c);
                    plot(px, py + 1, c);
                    plot(px + 1, py + 1, c);
                }
            }

            prev_x = (float)cur_x;
            prev_y = yy;
        }
    }

    tex->Unmap(0);

    const double alpha = 2.0 / (60.0 * 5 + 1.0);

    avg_level = (max_level - avg_level) * alpha + avg_level;
    avg_fft_level = (max_fft_level - avg_fft_level) * alpha + avg_fft_level;
    var_proj->SetMatrix((float*)&matrixProj);
    var_view->SetMatrix((float*)&matrixView);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    d3dDev->IASetInputLayout(vertex_layout);
    d3dDev->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
    d3dDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    D3DXMatrixTranslation(&matrix_world, x, y - 0, z);
    var_world->SetMatrix((float*)&matrix_world);
    technique->GetPassByIndex(0)->Apply(0);
    d3dDev->Draw(4, 0);


    c_stats::draw();
}