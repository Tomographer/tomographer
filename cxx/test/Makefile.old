

all: test_integrator_basic #test_lib_basics


CXX = g++

OPTIM=-O3 -msse2 -msse3 -mssse3 -msse4.1 -msse4.2
#OPTIM=-O2
#OPTIM=-g
#OPTIM=-g -O3 -msse2 -msse3 -mssse3 -msse4.1 -msse4.2

EIGENINCDIR = /scratch/pfaist/soft/local/include/eigen3

CXXFLAGS =  $(OPTIM) -std=c++0x \
	 -I.. \
	 -I$(EIGENINCDIR) -I$(HOME)/.local/include -I. \
	 -Wall -Wunused -Wextra
LDFLAGS = -L$(CUBADIR)/lib -L$(HOME)/.local/lib  -lmatio -lz -lcblas -latlas -lm


test_lib_basics: test_lib_basics.cxx
	g++ -o $@ $<  -fopenmp  $(CXXFLAGS) -L$(HOME)/.local/lib -lboost_program_options -lmatio -lz -lm -lgomp

test_integrator_basic: test_integrator_basic.cxx
	g++ -o $@ $<  -fopenmp  $(CXXFLAGS) -L$(HOME)/.local/lib -lboost_program_options -lmatio -lz -lm -lgomp
