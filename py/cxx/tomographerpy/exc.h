
#ifndef TOMOGRAPHERPY_EXC_H
#define TOMOGRAPHERPY_EXC_H

#include <tomographerpy/common.h>


// thanks http://stackoverflow.com/a/9690436/1694896
inline PyObject* createExceptionClass(const char* name, PyObject* baseTypeObj = PyExc_Exception,
                                      std::string docstring = "Exception class")
{
  using std::string;
  namespace bp = boost::python;
  
  string scopeName = bp::extract<string>(bp::scope().attr("__name__"));
  string qualifiedName0 = scopeName + "." + name;
  char* qualifiedName1 = const_cast<char*>(qualifiedName0.c_str());
  
  PyObject* typeObj = PyErr_NewExceptionWithDoc(qualifiedName1, const_cast<char*>(docstring.c_str()), baseTypeObj, 0);
  if (typeObj == NULL) {
    bp::throw_error_already_set();
  }

  auto bpObj = bp::object(bp::handle<>(bp::borrowed(typeObj)));

  // add to current module
  bp::scope().attr(name) = bpObj;

  return typeObj;
}





#endif
