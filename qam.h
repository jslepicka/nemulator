#pragma once
#include "task.h"
#include <string>
#include "d3d10.h"
#include <d3dx10.h>

class c_qam :
	public c_task
{
public:
	c_qam();
	~c_qam();
	void init(void *params);
	int update(double dt, int child_result, void *params);
	void draw();
	void resize();
	void load_fonts();
	void set_valid_chars(int *v);
	void set_char(char c);
	void activate();
private:
	ID3DX10Font *font;
	int selected;
	static const char *c;
	double scroll_timer;
	double scroll_pos;
	static const double scroll_delay;
	int scroll_target;
	int state;
	int result;
	enum STATE
	{
		STATE_ACTIVATED = 0,
		STATE_SCROLL_IN,
		STATE_SCROLL_OUT,
		STATE_READY,
		STATE_IDLE
	};
	int *valid_chars;
};
