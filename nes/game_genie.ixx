module;

export module nes:game_genie;
import std.compat;

namespace nes
{

class c_game_genie
{
  public:
    c_game_genie();
    ~c_game_genie();
    int add_code(std::string code);
    int remove_code(int index);
    uint8_t filter_read(uint16_t address, uint8_t value);
    int count; //number of active codes
  private:
    static const int char_to_int[];
    struct s_code
    {
        int address;
        int value;
        int comparison;
    };
    std::vector<s_code> codes;
};

} //namespace nes