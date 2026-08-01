CXX = arm-linux-gnueabihf-g++

CXXFLAGS = -Wall -O2 -std=c++17

INC = -Iinclude

SRC = \
src/main.cpp \
src/RegisterController.cpp

OUT = bin/sdr_app

all:

	$(CXX) $(CXXFLAGS) $(INC) $(SRC) -o $(OUT)

clean:

	rm -f $(OUT)
