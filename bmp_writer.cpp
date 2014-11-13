#include "bmp_writer.h"

const unsigned char c_bmp_writer::magic[] = { 'B', 'M' };

int c_bmp_writer::write_bmp(int *source, int x, int y, std::string filename)
{
	s_header header = {0};
	s_dib_header dib_header = {0};
	int pal[256] = {0};

	dib_header.header_size = sizeof(dib_header);
	dib_header.width = x;
	dib_header.height = -y;
	dib_header.num_planes = 1;
	dib_header.bits_pp = 8;
	dib_header.bmp_bytes = (x*y);

	std::ofstream file;
	file.open(filename, std::ios_base::trunc | std::ios_base::binary);
	if (file.is_open())
	{
		unsigned char *temp = new unsigned char[x*y];
		int palette_index = 0;
		std::map <int, int> pal_map;

		
		int *s = source;
		unsigned char *dest = temp;
		for (int yy = 0; yy < y; yy++)
		{
			//s = source + ((y-1) - yy) * x;
			for (int xx = 0; xx < x; xx++)
			{
				int r = (*s >> 16) & 0xFF; 
				int g = (*s >> 8) & 0xFF;
				int b = (*s) & 0xFF;
				int c = r | (g << 8) | (b << 16);

				if (pal_map.count(c) > 0)
				{
					*dest++ = pal_map[c];
				}
				else
				{
					if (palette_index < 256)
					{
						pal_map.insert(std::pair<int, int>(c, palette_index));
						*dest++ = palette_index;
						palette_index++;
					}
					else
						*dest++ = 0;
				}
				s++;
			}
		}

		for (auto &palette_entry : pal_map)
		{
			pal[palette_entry.second] = palette_entry.first;
		}

		header.file_size = sizeof(magic) + sizeof(header) + sizeof(dib_header) + (palette_index * 4) + (x*y);
		header.bmp_offset = sizeof(magic) + sizeof(header) + sizeof(dib_header) + (palette_index * 4);
		dib_header.num_colors = palette_index;

		file.write((char*)magic, sizeof(magic));
		file.write((char*)&header, sizeof(header));
		file.write((char*)&dib_header, sizeof(dib_header));
		file.write((char*)pal, palette_index * 4);
		file.write((char*)temp, x*y);
		file.close();

		delete[] temp;
		return 0;
	}
	else
		return 1;
}