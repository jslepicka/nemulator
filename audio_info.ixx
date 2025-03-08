module;

#include <d3d10.h>
#include <D3DX10.h>
export module nemulator:audio_info;
import task;

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
        D3DXVECTOR3 pos;
        D3DXVECTOR2 tex;
    };
    ID3D10Effect *effect;
    ID3D10Texture2D *tex;
    ID3D10EffectTechnique *technique;
    ID3D10EffectShaderResourceVariable *var_tex;
    ID3D10Buffer *vertex_buffer;
    ID3D10ShaderResourceView *tex_rv;
    ID3D10InputLayout *vertex_layout;
    D3DXMATRIX matrix_world;

        ID3D10EffectMatrixVariable *var_world;
    ID3D10EffectMatrixVariable *var_view;
    ID3D10EffectMatrixVariable *var_proj;
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