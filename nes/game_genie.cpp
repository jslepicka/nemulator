#include "game_genie.h"

const int c_game_genie::char_to_int[] = {
	//A  B   C   D   E   F   G   H   I   J   K   L   M
	0,  -1, -1, -1,  8, -1,  4, -1,  5, -1, 12,  3, -1,
	//N  O   P   Q   R   S   T   U   V   W   X   Y   Z
	 15, 9,  1, -1, -1, 13,  6, 11, 14, -1,  10, 7,  2,
};

c_game_genie::c_game_genie()
{
	count = 0;
}

c_game_genie::~c_game_genie()
{

}

int c_game_genie::add_code(std::string code)
{
	if (!(code.length() == 6 || code.length() == 8)) {
		return -1;
	}
	enum {
		TYPE_VALUE,
		TYPE_ADDRESS,
		TYPE_COMPARE,
		TYPE_IGNORE
	};
	struct s_mapping {
		int dest_bit;
		int type;
	};

	s_mapping mapping[2][8][4] = {
		{
			{
				{7, TYPE_VALUE}, {6, TYPE_VALUE}, {5, TYPE_VALUE}, {0, TYPE_VALUE}
			},
			{
				{3, TYPE_VALUE}, {2, TYPE_VALUE}, {1, TYPE_VALUE}, {7, TYPE_ADDRESS}
			},
			{
				{10, TYPE_ADDRESS}, {9, TYPE_ADDRESS}, {8, TYPE_ADDRESS}, {7, TYPE_IGNORE}
			},
			{
				{2, TYPE_ADDRESS}, {1, TYPE_ADDRESS}, {0, TYPE_ADDRESS}, {11, TYPE_ADDRESS}
			},
			{
				{14, TYPE_ADDRESS}, {13, TYPE_ADDRESS}, {12, TYPE_ADDRESS}, {3, TYPE_ADDRESS}
			},
			{
				{6, TYPE_ADDRESS}, {5, TYPE_ADDRESS}, {4, TYPE_ADDRESS}, {4, TYPE_VALUE}
			}
		},
		{
			{
				{7, TYPE_VALUE}, {6, TYPE_VALUE}, {5, TYPE_VALUE}, {0, TYPE_VALUE}
			},
			{
				{3, TYPE_VALUE}, {2, TYPE_VALUE}, {1, TYPE_VALUE}, {7, TYPE_ADDRESS}
			},
			{
				{10, TYPE_ADDRESS}, {9, TYPE_ADDRESS}, {8, TYPE_ADDRESS}, {7, TYPE_IGNORE}
			},
			{
				{2, TYPE_ADDRESS}, {1, TYPE_ADDRESS}, {0, TYPE_ADDRESS}, {11, TYPE_ADDRESS}
			},
			{
				{14, TYPE_ADDRESS}, {13, TYPE_ADDRESS}, {12, TYPE_ADDRESS}, {3, TYPE_ADDRESS}
			},
			{
				{6, TYPE_ADDRESS}, {5, TYPE_ADDRESS}, {4, TYPE_ADDRESS}, {4, TYPE_COMPARE}
			},
			{
				{7, TYPE_COMPARE}, {6, TYPE_COMPARE}, {5, TYPE_COMPARE}, {0, TYPE_COMPARE}
			},
			{
				{3, TYPE_COMPARE}, {2, TYPE_COMPARE}, {1, TYPE_COMPARE}, {4, TYPE_VALUE}
			}
		}
	};

	int address = 0;
	int value = 0;
	int compare = 0;

	auto process_char = [&](int char_num, char c) {

	};

	int char_num = 0;
	int table = code.length() == 6 ? 0 : 1;

	for (auto c : code) {
		int x = char_to_int[c - 65];
		for (int i = 0; i < 4; i++) {
			int bit = x & 1;
			switch (mapping[table][char_num][i].type) {
			case TYPE_VALUE:
				value |= (bit << (7 - mapping[table][char_num][i].dest_bit));
				break;
			case TYPE_ADDRESS:
				address |= (bit << (14 - mapping[table][char_num][i].dest_bit));
				break;
			case TYPE_COMPARE:
				compare |= (bit << (7 - mapping[table][char_num][i].dest_bit));
				break;
			case TYPE_IGNORE:
				break;
			default:
				break;
			}
			x >>= 1;
		}
		char_num++;
	}

	if (code.length() == 6) {
		compare = -1;
	}
	
	address += 0x8000;
	codes.push_back({ address, value, compare });
	count++;
	return count;
}

int c_game_genie::remove_code(int index)
{
	return -1;
}

uint8_t c_game_genie::filter_read(uint16_t address, uint8_t value)
{
	for (auto code : codes) {
		if (code.address == address) {
			if (code.comparison != -1) {
				if (value == code.comparison) {
					return code.value;
				}
			}
			return code.value;
		}
	}
	return value;
}