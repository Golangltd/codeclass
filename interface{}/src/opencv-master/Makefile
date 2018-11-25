# Copyright 2014 <chaishushan{AT}gmail.com>. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

TARG=opencv110

CXCORE_DIR=./opencv110/cxcore
CXCORE_INC=-I$(CXCORE_DIR)/include -I$(CXCORE_DIR)/src
CXCORE_CPP=$(wildcard $(CXCORE_DIR)/src/*.cpp)
CXCORE_OBJ=$(patsubst %.cpp,%.o,$(CXCORE_CPP))

CV_DIR=./opencv110/cv
CV_INC=-I$(CV_DIR)/include -I$(CV_DIR)/src
CV_CPP=$(wildcard $(CV_DIR)/src/*.cpp)
CV_OBJ=$(patsubst %.cpp,%.o,$(CV_CPP))

CVAUX_DIR=./opencv110/cvaux
CVAUX_INC=-I$(CVAUX_DIR)/include -I$(CVAUX_DIR)/src
CVAUX_CPP=$(wildcard $(CVAUX_DIR)/src/*.cpp)
CVAUX_OBJ=$(patsubst %.cpp,%.o,$(CVAUX_CPP))

ML_DIR=./opencv110/ml
ML_INC=-I$(ML_DIR)/include -I$(ML_DIR)/src
ML_CPP=$(wildcard $(ML_DIR)/src/*.cpp)
ML_OBJ=$(patsubst %.cpp,%.o,$(ML_CPP))

HIGHGUI_DIR=./opencv110/highgui
HIGHGUI_INC=-I$(HIGHGUI_DIR)/include -I$(HIGHGUI_DIR)/src
HIGHGUI_CPP=$(wildcard $(HIGHGUI_DIR)/src/*.cpp) $(wildcard $(HIGHGUI_DIR)/src/windows/*.cpp)
HIGHGUI_OBJ=$(patsubst %.cpp,%.o,$(HIGHGUI_CPP))

VIDEOINPUT_DEF=$(HIGHGUI_DIR)/src/windows/videoInput_capi.def

OPENCV_INC=$(CXCORE_INC) $(CV_INC) $(CVAUX_INC) $(ML_INC) $(HIGHGUI_INC)
OPENCV_OBJ=$(CXCORE_OBJ) $(CV_OBJ) $(CVAUX_OBJ) $(ML_OBJ) $(HIGHGUI_OBJ)

CC=g++
CCFLAGS=$(OPENCV_INC) -DSTRSAFE_NO_DEPRECATE -DCVAPI_EXPORTS
LDFLAGS=-lGdi32 -lOle32 -lStrmiids -lOleAut32 -lUuid -lComctl32 -lVfw32

default: $(OPENCV_OBJ) $(VIDEOINPUT_DEF)
	$(CC) -shared -o $(TARG).dll $(OPENCV_OBJ) $(VIDEOINPUT_DEF) -Wl,--out-implib,lib$(TARG).a $(LDFLAGS) 

clean:
	-rm $(OPENCV_OBJ)
	-rm *.dll
	-rm *.a

%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@

