module;
#include <string>
#include <fstream>
#include <map>

export module config;

export class c_config
{
public:
    c_config();
    ~c_config();
    bool read_config_file(std::string filename);
    bool get_string(std::string key, std::string *value);
    std::string get_string(std::string key, std::string default_value = "");

    bool get_int(std::string key, int *value);
    int get_int(std::string key, int default_value = 0);
    
    bool get_double(std::string key, double *value);
    double get_double(std::string key, double default_value = 0.0);

    bool get_bool(std::string key, bool default_value = false);

private:
    std::ifstream file;
    std::map<std::string, std::string> config;
    bool get_pair(std::string key, std::pair<std::string, std::string> *p);
    std::string trim (std::string s);
};