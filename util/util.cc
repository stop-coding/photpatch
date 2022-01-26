#include <sys/types.h>
#include "util.h"
#include <regex>

using namespace std;

vector<string>  string_split(const string& str,const string& delim) 
{
	vector<string> res;
	if(str.size() == 0) {
		return res;
	}
	
	string strs = str + delim; 
	size_t pos;
	size_t size = strs.size();
 
	for (size_t i = 0; i < size; ++i) {
		pos = strs.find(delim, i);
		if( pos < size) { 
			string s = strs.substr(i, pos - i);
			if (s.size() > 0) {
				res.push_back(s);
			}
			i = pos + delim.size() - 1;
		}
	}
	return res;	
}

static std::string  __inter_string_strip(const std::string& str, char c)
{
	int i = 0;
	if (str.size() == 0) {
		return "";
	}
	while (str[i] == c || str[i] == '\n' || str[i] == '\t')// 头部ch字符个数是 i
		i++;
	int j = str.size() - 1;
	while (str[j] == c || str[i] == '\n' || str[i] == '\t') //
		j--;		
	return str.substr(i, j + 1 -i );
}

std::string  string_strip(const std::string& str, char c)
{
	return __inter_string_strip(str, c);
}

std::string  string_strip(const std::string& str)
{		
	return __inter_string_strip(str, ' ');
}

/* vector<string> testSplit11(const string& in, const string& delim)
{
    vector<string> ret;
    try
    {
        regex re{delim};
        return vector<string>{
                sregex_token_iterator(in.begin(), in.end(), re, -1),
                sregex_token_iterator()
           };      
    }
    catch(const std::exception& e)
    {
        cout<<"error:"<<e.what()<<endl;
    }
    return ret;
} */