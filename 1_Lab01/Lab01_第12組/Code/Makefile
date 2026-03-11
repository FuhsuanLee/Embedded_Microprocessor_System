CROSS_COMPILE=aarch64-linux-gnu-
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
CXXFLAGS=-std=c++11
OBJ=main.o sort.o triangle.o ellipse.o circular_sector.o
SRC=main.cpp sort.cpp triangle.cpp ellipse.cpp circular_sector.cpp
EXE=tx2_exe
all:$(EXE)
$(EXE):$(OBJ)
	$(CXX) -o $@ $(OBJ)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
clean:
	rm -f $(EXE)
	rm -f $(OBJ)