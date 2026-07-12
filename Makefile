CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra
INCLUDES = -I$(HOME)/code/raylib/src
LIBS = $(HOME)/code/raylib/build/raylib/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

SRC = src/main.cpp
TARGET = build/main

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) $(LIBS) -o $(TARGET)

clean:
	rm -rf build/*

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run