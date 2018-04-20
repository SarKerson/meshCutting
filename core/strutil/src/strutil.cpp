#include "../include/strutil.h"
// declaration
namespace strutil {

using namespace std;

inline std::string trimLeft(const std::string& str)
{
    string t = str;
    t.erase(0, t.find_first_not_of(" \t\n\r"));
    return t;
};

inline std::string trimRight(const std::string& str)
{
    string t = str;
    t.erase(t.find_last_not_of(" \t\n\r") + 1);
    return t;
};

inline std::string trim(const std::string& str)
{
  string t = str;
    t.erase(0, t.find_first_not_of(" \t\n\r"));
    t.erase(t.find_last_not_of(" \t\n\r") + 1);
    return t;
};

inline std::string trim(const std::string& str, const std::string & delimitor)
{
    string t = str;
    t.erase(0, t.find_first_not_of(delimitor));
    t.erase(t.find_last_not_of(delimitor) + 1);
    return t;
};
 
inline  std::string toLower(const std::string& str)
{
    string t = str;
    transform(t.begin(), t.end(), t.begin(),  (int (*)(int))tolower);
    return t;
};

inline  std::string toUpper(const std::string& str)
{
    string t = str;
    transform(t.begin(), t.end(), t.begin(),  (int (*)(int))toupper);
    return t;
};

inline  bool startsWith(const std::string& str, const std::string& substr)
{
  return str.find(substr) == 0;
};
  
inline bool endsWith(const std::string& str, const std::string& substr)
{
  return (str.rfind(substr) == (str.length() - substr.length()) && str.rfind(substr) >= 0 );
};

inline bool equalsIgnoreCase(const std::string& str1, const std::string& str2)
{
  return toLower(str1) == toLower(str2);
};


inline    std::string toString(const bool& value)
{
    ostringstream oss;
    oss << boolalpha << value;
    return oss.str();
};

}


namespace strutil {

inline std::vector<std::string> split(const std::string& str, const std::string& delimiters)
{
        vector<string> ss;

        Tokenizer tokenizer(str, delimiters);
        while (tokenizer.nextToken()) 
    {
            ss.push_back(tokenizer.getToken());
        }

        return ss;
};

};
/*
struct string_token_iterator 
  : public std::iterator<std::input_iterator_tag, std::string>
{
public:
    string_token_iterator() : str(0), start(0), end(0) {}
    string_token_iterator(const std::string & str_, const char * separator_ = " ") :
    separator(separator_),
    str(&str_),
    end(0)
    {
      find_next();
    }
    string_token_iterator(const string_token_iterator & rhs) :
    separator(rhs.separator),
    str(rhs.str),
    start(rhs.start),
    end(rhs.end)
  {
  }

  string_token_iterator & operator++()
  {
    find_next();
    return *this;
  }

  string_token_iterator operator++(int)
  {
    string_token_iterator temp(*this);
    ++(*this);
    return temp;
  }

  std::string operator*() const
  {
    return std::string(*str, start, end - start);
  }

  bool operator==(const string_token_iterator & rhs) const
  {
    return (rhs.str == str && rhs.start == start && rhs.end == end);
  }

  bool operator!=(const string_token_iterator & rhs) const
  {
    return !(rhs == *this);
  }

private:

  void find_next(void)
  {
    start = str->find_first_not_of(separator, end);
    if(start == std::string::npos)
    {
      start = end = 0;
      str = 0;
      return;
    }

    end = str->find_first_of(separator, start);
  }

  const char * separator;
  const std::string * str;
  std::string::size_type start;
  std::string::size_type end;
};
*/

