module;
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <utility>

export module config;

export class c_config
{
public:
    c_config();
    ~c_config();
    bool read_config_file(std::string filename);
    //rewrites the config file, deleting the lines for the keys in remove and appending the values at
    //the end; comments and all other lines are preserved
    bool update_config_file(const std::vector<std::string> &remove,
                            const std::vector<std::pair<std::string, std::string>> &append);
    //changes the values of keys where they are in the config file, leaving every other line as it is;
    //keys that aren't in the file are appended at the end
    bool set_config_values(const std::vector<std::pair<std::string, std::string>> &values);
    bool get_string(std::string key, std::string *value);
    std::string get_string(std::string key, std::string default_value = "");

    bool get_int(std::string key, int *value);
    int get_int(std::string key, int default_value = 0);
    
    bool get_double(std::string key, double *value);
    double get_double(std::string key, double default_value = 0.0);

    bool get_bool(std::string key, bool default_value = false);

private:
    std::string config_filename;
    bool read_lines(std::vector<std::string> *lines);
    bool write_lines(const std::vector<std::string> &lines);
    std::string get_line_key(const std::string &line);
    void append_lines(std::vector<std::string> &lines, const std::vector<std::pair<std::string, std::string>> &values);
    std::ifstream file;
    std::map<std::string, std::string> config;
    bool get_pair(std::string key, std::pair<std::string, std::string> *p);
    std::string trim (std::string s);
};