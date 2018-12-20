#pragma once
#include "TexturePanelItem.h"
#include <vector>
#include <list>
#include "d3d10.h"
#include "d3dx10.h"
//#include "d3d10app.h"

#define ReleaseCOM(x) { if(x) {x->Release(); x = 0; } }

#include <crtdbg.h>
#if defined(DEBUG) | defined(_DEBUG)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

class TexturePanel
{
public:
	TexturePanel(int rows, int columns);
	~TexturePanel(void);
	void load_items();
	bool is_first_col();
	bool is_last_col();
	int get_num_items();
	void set_scroll_duration(float duration) { scrollDuration = duration; };
	void Init();
	void AddItem(TexturePanelItem *item);
	void Update(double dt);
	void Draw(/*float x, float y, float z*/);
	void GetActive(std::list<TexturePanelItem*> *itemList);
	int NextRow(bool adjusting = false);
	int PrevRow(bool adjusting = false);
	void NextColumn(bool load = true, bool adjusting = false);
	void PrevColumn(bool load = true, bool adjusting = false);
	void move_to_column(int column);
	void move_to_char(char c);
	void Zoom();
	bool Changed();
	bool selected; //is this panel selected?
	int GetSelectedColumn();
	TexturePanelItem* GetSelected();
	TexturePanel *prev;
	TexturePanel *next;
	float x;
	float y;
	float z;
	float zoomDestX;
	float zoomDestY;
	float zoomDestZ;
	void Suspend();
	void Resume();
	void OnResize();
	float zoomedR;
	float zoomedG;
	float zoomedB;

	static const int STATE_NULL = 0;
	static const int STATE_MENU = 1;
	static const int STATE_SCROLLING = 2;
	static const int STATE_ZOOMING = 3;
	static const int STATE_ZOOMED = 4;
	static const int STATE_SUSPENDED = 5;
	int state;
	bool dim;
	bool selectable;
	float camera_distance;
	int *get_valid_chars();
	bool stretch_to_fit;
	void set_sharpness(float factor);

private:

	struct SimpleVertex
	{
		D3DXVECTOR3 pos;
		D3DXVECTOR2 tex;
	};
	class c_item_container
	{
	public:
		c_item_container(TexturePanelItem *item);
		~c_item_container();
		D3DXVECTOR3 pos;
		float ratio;
		bool selecting;
		bool selected;
		void Select();
		void Unselect();
		double selectTimer;
		static const float selectDuration;
		int selectDir;
		TexturePanelItem *item;
	};

	int scrolls;

	bool changed; //has the panel's items changed?
	bool prevSelected; //previous selected state

	int rows;
	int columns;


	int prevState;

	int numItems;

	bool on_first_page; //are we on the first page of this panel?
	int first_item; //index to the first item on the current page
	int last_item; //index to the last item on the current page
	int selected_item; //index to the currently selected item
	//int prevSelectedItem; //index to the previously selected item

	void DrawItem(c_item_container *item, int draw_border, float x, float y, float z, float c = 1.0f);

	std::vector<c_item_container*> item_containers;



	double scrollDuration;
	static const float zoomDuration;
	static const float borderDuration;


	int scrollDir;
	double scrollTimer;
	bool scrollCleanup;
	bool scrollRepeat;

	double zoomTimer;
	int zoomDir;
	float zoomStartX;
	float zoomStartY;
	float zoomStartZ;

	double borderTimer;
	int borderDir;

	float borderR;
	float borderG;
	float borderB;

	float borderInvalidR;
	float borderInvalidG;
	float borderInvalidB;

	float borderR1;
	float borderG1;
	float borderB1;
	float borderR2;
	float borderG2;
	float borderB2;

	float borderInvalidR1;
	float borderInvalidG1;
	float borderInvalidB1;
	float borderInvalidR2;
	float borderInvalidG2;
	float borderInvalidB2;

	HRESULT hr;

	ID3D10Effect *effect;
	ID3D10Texture2D *tex;
	ID3D10Texture2D *tex_blank;
	ID3D10EffectTechnique *technique;
	ID3D10EffectMatrixVariable *varWorld;
	ID3D10EffectMatrixVariable *varView;
	ID3D10EffectMatrixVariable *varProj;
	ID3D10EffectScalarVariable *varColor;
	ID3D10EffectVectorVariable *var_output_size;
	ID3D10EffectVectorVariable *var_border_color;
	ID3D10EffectVectorVariable *var_overscan_color;
	ID3D10EffectScalarVariable *var_sharpness;
	ID3D10EffectScalarVariable *var_max_y;
	ID3D10EffectScalarVariable *var_max_x;

	ID3D10EffectShaderResourceVariable *varTex;
	//ID3D10EffectShaderResourceVariable *varTex2;

	ID3D10Buffer *vertexBuffer;
	ID3D10Buffer *vertexBuffer2;
	ID3D10ShaderResourceView *texRv;
	//ID3D10ShaderResourceView *tex2Rv;
	ID3D10InputLayout *vertexLayout;

	D3DXMATRIX matrixWorld;

	static float InterpolateLinear(float start, float end, float mu);
	static float InterpolateCosine(float start, float end, float mu);

	struct float3 {
		float x;
		float y;
		float z;
	};

	static D3DXVECTOR3 interpolate_linear3(D3DXVECTOR3 &start, D3DXVECTOR3 &end, float mu);
	static D3DXVECTOR3 interpolate_cosine3(D3DXVECTOR3 &start, D3DXVECTOR3 &end, float mu);

	int valid_chars[27];

	D3D10_BUFFER_DESC bd;
	SimpleVertex vertices2[4];

	void build_stretch_buffer(float ratio);



	/*std::vector<float> border_color(3);
	std::vector<float> border_color_start(3);
	std::vector<float> border_color_end(3);*/

	D3DXVECTOR3 border_color, border_color_start, border_color_end;
	D3DXVECTOR3 invalid_border_color, invalid_border_color_start, invalid_border_color_end;
};
