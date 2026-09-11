module;
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>

module config;

c_config::c_config()
{
}

c_config::~c_config()
{
    if (file.is_open())
        file.close();
}

std::string c_config::trim(std::string s)
{
    if (s.length() == 0 || (s.length() == 1 && s[0] < 33))
        return "";
    int start = 0;
    while (start < (int) s.length() && s[start++] < 33);
    if (start > 0) start--;
    int end = (int)s.length() - 1;
    while (end >= 0 && s[end--] < 33);
    end++;
    return s.substr(start, end-start + 1);
}

bool c_config::read_config_file(std::string filename)
{
    config_filename = std::filesystem::absolute(filename).string();
    file.open(filename.c_str());
    if (!file.is_open())
        return false;
    char temp[256];
    std::string line;
    while (!file.eof())
    {
        file.getline(temp, 256);
        if (!(file.rdstate() & std::ios::badbit))
        {
            line = trim(temp);
            if (line.length() == 0 || line[0] == ';')
                continue;

            std::basic_string <char>::size_type i;

            i = line.find("=");
            if (i != std::string::npos)
            {
                int start = 0;
                std::string key = trim(line.substr(0, i));
                std::string value = trim(line.substr(i+1));
                config[key] = value;
            }
        }
    }
    file.close();
    return true;
}

bool c_config::update_config_file(const std::vector<std::string> &remove,
                                  const std::vector<std::pair<std::string, std::string>> &append)
{
    std::set<std::string> replaced(remove.begin(), remove.end());
    for (auto &a : append)
        replaced.insert(a.first);

    std::vector<std::string> lines;
    if (std::filesystem::exists(config_filename))
    {
        std::ifstream in(config_filename);
        if (!in.is_open())
            return false;
        std::string line;
        while (std::getline(in, line))
        {
            std::string trimmed = trim(line);
            std::string::size_type i = trimmed.find("=");
            if (trimmed.length() > 0 && trimmed[0] != ';' && i != std::string::npos &&
                replaced.count(trim(trimmed.substr(0, i))))
                continue;
            lines.push_back(line);
        }
    }

    //appended values are separated from the rest of the file by a single blank line.  Trailing blank
    //lines are only trimmed when appending, so removing values restores the file as it was.
    if (append.size() > 0)
    {
        while (lines.size() > 0 && trim(lines.back()).length() == 0)
            lines.pop_back();
        lines.push_back("");
        for (auto &a : append)
            lines.push_back(a.first + " = " + a.second);
    }

    //write to a temporary file first so that a failed write can't damage the original
    std::string temp_filename = config_filename + ".tmp";
    std::ofstream out(temp_filename);
    if (!out.is_open())
        return false;
    for (auto &l : lines)
        out << l << "\n";
    out.close();
    std::error_code ec;
    if (out.fail())
    {
        std::filesystem::remove(temp_filename, ec);
        return false;
    }
    std::filesystem::rename(temp_filename, config_filename, ec);
    if (ec)
        return false;

    for (auto &key : remove)
        config.erase(key);
    for (auto &a : append)
        config[a.first] = a.second;
    return true;
}

bool c_config::get_int(std::string key, int *value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return false;
    *value = atoi(((std::string)(p.second)).c_str());
    return true;
}

int c_config::get_int(std::string key, int default_value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return default_value;
    else
    {
        std::string blah = (std::string)(p.second);
        const char *value = blah.c_str();
        if (memcmp(value, "0x", 2) == 0)
        {
            char *c = (char*)value + strlen(value) - 1;

            int v = 0;
            int shift = 0;
            while (*c != 'x')
            {
                if (*c >= '0' && *c <= '9')
                    v += ((*c - '0') << 4*shift);
                else if (toupper(*c) >= 'A' && toupper(*c) <= 'F')
                    v += ((toupper(*c) - 'A' + 10) << 4*shift);
                else
                    return default_value;
                c--;
                shift++;
            }
            return v;
        }
        return atoi(((std::string)(p.second)).c_str());
    }
}

bool c_config::get_string(std::string key, std::string *value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return false;
    *value = ((std::string)(p.second));
    return true;
}

std::string c_config::get_string(std::string key, std::string default_value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return default_value;
    return ((std::string)(p.second));
}

bool c_config::get_double(std::string key, double *value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return false;
    else
    *value = atof(((std::string)(p.second)).c_str());
    return true;
}

double c_config::get_double(std::string key, double default_value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return default_value;
    else
        return atof(((std::string)(p.second)).c_str());
}

bool c_config::get_bool(std::string key, bool default_value)
{
    std::pair<std::string, std::string> p;
    if (!get_pair(key, &p))
        return default_value;
    if (_stricmp(((std::string)(p.second)).c_str(), "true") == 0)
        return true;
    return false;
}

bool c_config::get_pair(std::string key, std::pair<std::string,std::string> *p)
{
    std::map<std::string, std::string>::iterator i;
    i = config.find(key);
    if (i == config.end())
        return false;
    *p = *i;
    return true;
}
