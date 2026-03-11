CROSS_COMPILE=aarch64-linux-gnu-
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
CXXFLAGS=-std=c++11

TARGET = L2Program
SRC = lab2.cpp

$(TARGET):$(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
