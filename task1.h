#pragma once
#include "task.h"
#include <string>
#include "d3d10.h"
#include <d3dx10.h>

class c_task1 :
	public c_task
{
public:
	c_task1();
	~c_task1();
	void init(void *params);
	int update(double dt, int child_result, void *params);
	void draw();
private:
	c_task *n;
	ID3DX10Font *font;
	double elapsed;
	int counter;
	std::string s;
	int xx;
	int ch;
};
