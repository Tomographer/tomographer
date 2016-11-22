

EIGEN_INCLUDE_DIR = -I/usr/local/include/eigen3

BOOST_INCLUDE_DIR = -I/usr/local/include

TOMOGRAPHER_INCLUDE_DIR = -I/opt/tomographer-install/include




EXTRA_CFLAGS =  -std=c++11 -fdiagnostics-color -fopenmp

COMPILE = /opt/gcc4.9/bin/gcc -fno-strict-aliasing -fno-common -dynamic -g -O2 -Wall -Wextra $(BOOST_INCLUDE_DIR) $(EIGEN_INCLUDE_DIR) $(TOMOGRAPHER_INCLUDE_DIR)  -I. -I/usr/local/include -I/usr/local/opt/openssl/include -I/usr/local/opt/sqlite/include -I/usr/local/Cellar/python/2.7.12_1/Frameworks/Python.framework/Versions/2.7/include/python2.7 $(EXTRA_CFLAGS)

LINK = /opt/gcc4.9/bin/g++ -bundle -undefined dynamic_lookup build/temp.macosx-10.6-x86_64-2.7/pytomorun.o build/temp.macosx-10.6-x86_64-2.7/tomographerbase.o build/temp.macosx-10.6-x86_64-2.7/pyhistogram.o build/temp.macosx-10.6-x86_64-2.7/eigpyconv.o -L. -L/usr/local/lib -L/usr/local/opt/openssl/lib -L/usr/local/opt/sqlite/lib -lboost_python-mt  -fopenmp




default: build/tomographer.so


build/pyhistogram.o: pyhistogram.cxx common.h pyhistogram.h eigpyconv.h
	$(COMPILE) -c -o $@ $<

build/pytomorun.o: pytomorun.cxx common.h pyhistogram.h eigpyconv.h
	$(COMPILE) -c -o $@ $<

build/tomographerbase.o: tomographerbase.cxx common.h eigpyconv.h
	$(COMPILE) -c -o $@ $<

build/eigpyconv.o: eigpyconv.cxx common.h eigpyconv.h
	$(COMPILE) -c -o $@ $<


build/tomographer.so: build/pyhistogram.o build/pytomorun.o build/eigpyconv.o build/tomographerbase.o
	/opt/gcc4.9/bin/g++   -fopenmp -bundle -undefined dynamic_lookup -o $@  $^  -L. -L/usr/local/lib -L/usr/local/opt/openssl/lib  -L/usr/local/opt/sqlite/lib -lboost_python-mt
