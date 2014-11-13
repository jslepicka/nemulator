#pragma once

#include "task.h"
#include <d3d10.h>
#include <dxgi.h>
#include "D3DX10.h"

class c_mem_access_log;

class c_mem_viewer :
	public c_task
{
public:
	c_mem_viewer();
	~c_mem_viewer();
	void init (void *params);
	int update (double dt, int child_result, void *params);
	void draw();
	void set_mem_access_log(c_mem_access_log *log);
	//void resize();
	float x;
	float y;
	float z;
	int size;
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
	static const int tex_size = 256;
	c_mem_access_log *mem_access_log;
	float InterpolateLinear(float start, float end, float mu);
	int selected_location;
};