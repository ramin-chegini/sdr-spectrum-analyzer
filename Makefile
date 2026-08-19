CXX = arm-linux-gnueabihf-g++

CXXFLAGS = -Wall -O2 -std=c++17

LIBIIO_INC = third_party/libiio/include
LIBIIO_LIB = third_party/libiio/lib

INC = \
-Iinclude \
-I$(LIBIIO_INC)

LIBS = \
-L$(LIBIIO_LIB) \
-liio

SRC = \
src/main.cpp \
src/RegisterController.cpp \
src/dma.cpp \
src/ad9361.cpp \
src/adi_iio_capture.cpp \
src/l2_protocol.cpp

OUT = bin/sdr_app

all:
	$(CXX) $(CXXFLAGS) $(INC) $(SRC) $(LIBS) -o $(OUT)

clean:
	rm -f $(OUT)