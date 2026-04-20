export module genesis:db;
import nemulator.std;

namespace genesis
{

enum REGION
{
        US,
        JAPAN
};

struct s_db_entry
{
    REGION region;
};

export std::map<uint32_t, s_db_entry> db = {
    {0x90FA1539, {REGION::JAPAN}} // Alien Soldier (J) [!].bin
};

} //namespace genesis