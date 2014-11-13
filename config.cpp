#include "config.h"
#include <iostream>
using namespace std;

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
	int end = s.length() - 1;
	while (end >= 0 && s[end--] < 33);
	end++;
	return s.substr(start, end-start + 1);
}

bool c_config::read_config_file(std::string filename)
{
	file.open(filename.c_str());
	if (!file.is_open())
		return false;
	char temp[256];
	std::string line;
	while (!file.eof())
	{
		file.getline(temp, 256);
		if (!(file.rdstate() & ios::badbit))
		{
			line = trim(temp);
			if (line.length() == 0 || line[0] == ';')
				continue;

			basic_string <char>::size_type i;
			
			i = line.find("=");
			if (i != string::npos)
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
