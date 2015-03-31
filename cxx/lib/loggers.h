
#ifndef LOGGERS_H
#define LOGGERS_H

#include <string>
#include <cstdio>
#include <cstdarg>

#define PRINTF1_ARGS_SAFE  __attribute__ ((format (printf, 1, 2)))
//#define PRINTF1_ARGS_SAFE
#define PRINTF2_ARGS_SAFE  __attribute__ ((format (printf, 2, 3)))


class bad_fmtf_format : public std::exception
{
  std::string msg;
public:
  bad_fmtf_format(const std::string& msg_) : msg(msg_) { }
  ~bad_fmtf_format() throw() { }

  const char * what() const throw() {
    return msg.c_str();
  }
};


// check out printf() formatting for std::string:
//    http://stackoverflow.com/a/10150393/1694896
//    http://stackoverflow.com/a/26197300/1694896

inline std::string vfmtf(const char* fmt, va_list vl)
{
  int size = 10;
  char * buffer = new char[size];
  va_list ap1;
  va_copy(ap1, vl);
  int nsize = vsnprintf(buffer, size, fmt, ap1);
  if (nsize < 0) {
    // failure: bad format probably
    throw bad_fmtf_format("vsnprintf("+std::string(fmt)+") failure: code="+std::to_string(nsize));
  }
  if(size <= nsize) {
    // buffer too small: delete buffer and try again
    delete[] buffer;
    size = nsize+1; // +1 for "\0"
    buffer = new char[size];
    nsize = vsnprintf(buffer, size, fmt, vl);
  }
  std::string ret(buffer);
  delete[] buffer;
  va_end(ap1);
  return ret;
}


inline std::string fmtf(const char * fmt, ...)  PRINTF1_ARGS_SAFE;

inline std::string fmtf(const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  std::string result = vfmtf(fmt, ap);
  va_end(ap);
  return result;
}






class SimpleFoutLogger
{
public:
  SimpleFoutLogger(FILE * fp_)
    : fp(fp_)
  {
  }

  PRINTF2_ARGS_SAFE
  inline void longdebug(const char * fmt, ...);

private:
  std::FILE * fp;
};

PRINTF2_ARGS_SAFE
inline void SimpleFoutLogger::longdebug(const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(fp, "%s\n", vfmtf(fmt, ap).c_str());
  va_end(ap);
}




#endif
