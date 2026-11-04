module;
#include <d3d11.h>
#include <DirectXMath.h>

export module shader_pass;

//what transform_vs.hlsl expects in constant buffer 0
export struct s_transform_constants
{
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};

//a vertex shader, a pixel shader, and the state to draw with them.  constant buffers, textures, samplers, and the
//input assembler are set by whoever draws.
export class c_shader_pass
{
  public:
    struct s_desc
    {
        const BYTE *vertex_shader;
        SIZE_T vertex_shader_size;
        const BYTE *pixel_shader;
        SIZE_T pixel_shader_size;
        const D3D11_BLEND_DESC *blend;
        //without these, whatever is already set stays in effect
        const D3D11_DEPTH_STENCIL_DESC *depth = nullptr;
        const D3D11_RASTERIZER_DESC *raster = nullptr;
    };

    c_shader_pass() = default;
    ~c_shader_pass();
    c_shader_pass(const c_shader_pass &) = delete;
    c_shader_pass &operator=(const c_shader_pass &) = delete;

    bool create(const s_desc &desc);
    bool is_created() const { return pixel_shader != nullptr; }
    void apply();

  private:
    ID3D11VertexShader *vertex_shader = nullptr;
    ID3D11PixelShader *pixel_shader = nullptr;
    ID3D11BlendState *blend = nullptr;
    ID3D11DepthStencilState *depth = nullptr;
    ID3D11RasterizerState *raster = nullptr;
};

export D3D11_BLEND_DESC no_blend_desc();
export D3D11_BLEND_DESC alpha_blend_desc();
export ID3D11SamplerState *create_linear_sampler(D3D11_TEXTURE_ADDRESS_MODE address);
export ID3D11Buffer *create_constant_buffer(UINT size);

//T is written whole with UpdateSubresource, so its size has to be a valid size for a constant buffer
export template <typename T> ID3D11Buffer *create_constant_buffer()
{
    static_assert(sizeof(T) % 16 == 0);
    return create_constant_buffer((UINT)sizeof(T));
}
