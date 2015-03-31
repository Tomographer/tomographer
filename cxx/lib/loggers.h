
#ifndef LOGGERS_H
#define LOGGERS_H

#include <string>
#include <cstdio>

// check out printf() formatting for std::string:
//    http://stackoverflow.com/a/26197300/1694896

template <typename... Ts>
std::string fmt (const std::string &fmt, Ts... vs)
{
    char b;
    unsigned required = std::snprintf(&b, 0, fmt.c_str(), vs...) + 1;

    char bytes[required];
    std::snprintf(bytes, required, fmt.c_str(), vs...);

    return std::string(bytes);
}






class SimpleFoutLogger
{
public:
  SimpleFoutLogger(FILE * fp_)
    : fp(fp_)
  {
  }

  template<typename... Ts>
  void error(const std::string & fmt, Ts... vs)
  {
    fprintf(fp, "ERROR: %s\n", fmt(fmt, vs).c_str());
  }

  template<typename... Ts>
  void warning(const std::string & fmt, Ts... vs)
  {
    fprintf(fp, "Warning: %s\n", fmt(fmt, vs).c_str());
  }

  template<typename... Ts>
  void info(const std::string & fmt, Ts... vs)
  {
    fprintf(fp, "[Info] %s\n", fmt(fmt, vs).c_str());
  }

  template<typename... Ts>
  void debug(const std::string & fmt, Ts... vs)
  {
    fprintf(fp, "%s\n", fmt(fmt, vs).c_str());
  }

  template<typename... Ts>
  void longdebug(const std::string & fmt, Ts... vs)
  {
    fprintf(fp, "%s\n", fmt(fmt, vs).c_str());
  }

private:
  std::FILE * fp;
};




#endif
