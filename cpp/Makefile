#!/bin/make
CXX = g++
CXXFLAGS = -03 -Wall -g 
LDFLAGS = -lm
INCLUDES = /opt/anaconda3/include/python3.5m  `python3.5m-config --cflags --libs --extension-suffix`
LIBS = -lneo4j-client -lssl -lcrypto -lboost_iostreams -lboost_regex -lopenblas -llapack -fopenmp -lpthread
OUTPUT_FILE = hetinf_pmd.so
OBJECT_FILES = hetinf_pmd.cpp
CPPFLAGS += -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

$(OUTPUT_FILE) : $(OBJECT_FILES)
	$(CXX) -O3 -shared -march=native -std=c++14 -fPIC -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 $(OBJECT_FILES) -o $(OUTPUT_FILE) $(LIBS)

# -larmadillo 
#$(OUTPUT_FILE) : $(OBJECT_FILES)
#	$(CXX)  $(OBJECT_FILES) -o $(OUTPUT_FILE) $(LIBS)

#$(OUTPUT_FILE): $(OBJECT_FILES)
#	g++ $(OBJECT_FILES) -o $(OUTPUT_FILE)

#2012_2_27_main.o : 2012_2_27_main.cpp
#	g++ -c 2012_2_27_main.cpp
#2012_2_27_sample.o : 2012_2_27_sample.cpp
#	g++ -c 2012_2_27_sample.cpp



