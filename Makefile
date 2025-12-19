# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++11

# GStreamer configuration using pkg-config
GST_CFLAGS := $(shell pkg-config --cflags gstreamer-1.0)
GST_LIBS := $(shell pkg-config --libs gstreamer-1.0)

# Target executable name
TARGET = simple_player

# Source files
SRCS = main.cpp

# Rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(GST_CFLAGS) -o $(TARGET) $(SRCS) $(GST_LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
