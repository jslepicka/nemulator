#pragma once
#include <string>
#include <fstream>
#include <map>

class c_bmp_writer
{
public:
	static int write_bmp(int *source, int x, int y, std::string filename);
private:
	c_bmp_writer() {};
	virtual ~c_bmp_writer() {};

	inline static const unsigned char magic[2] = { 'B', 'M' };

	struct s_header
	{
		unsigned int file_size;
		unsigned short creator1;
		unsigned short creator2;
		unsigned int bmp_offset;
	};

	struct s_dib_header
	{
		unsigned int header_size;
		int width;
		int height;
		unsigned short num_planes;
		unsigned short bits_pp;
		unsigned int compress_type;
		unsigned int bmp_bytes;
		int hres;
		int yres;
		unsigned int num_colors;
		unsigned int num_important_colors;
	};
};

