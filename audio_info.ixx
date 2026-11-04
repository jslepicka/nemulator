module;

#include <d3d11.h>
#include <DirectXMath.h>
export module nemulator:audio_info;
import task;
import vec;
import shader_pass;

export class c_audio_info :
    public c_task
{
public:
    c_audio_info();
    ~c_audio_info();
    void init (void *params);
    int update (double dt, int child_result, void *params);
    void draw();
    void resize();
    void report(int freq, int buf, int stable, int max_freq, int min_freq);
    float x;
    float y;
    float z;
private:
        struct SimpleVertex
    {
        s_vec3 pos;
        s_vec2 tex;
    };
    c_shader_pass pass;
    s_transform_constants transform = {};
    ID3D11Buffer *transform_buffer = nullptr;
    ID3D11SamplerState *sampler = nullptr;
    ID3D11Texture2D *tex;
    ID3D11Buffer *vertex_buffer;
    ID3D11ShaderResourceView *tex_rv;
    ID3D11InputLayout *vertex_layout;

    int count;
    static const int tex_size = 256;
    int freq[tex_size];
    int buf[tex_size];
    int max_freq[tex_size];
    int min_freq[tex_size];
    int stable;
    int read_pointer;
    static const int MAX = 26680;
};