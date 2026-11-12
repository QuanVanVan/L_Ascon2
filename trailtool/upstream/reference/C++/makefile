CXX=g++

CXX_FLAGS=-g -g0 -O3 -Wall -Werror -Wextra -Wno-write-strings -Wno-deprecated-declarations -std=c++0x

BIN=bin
SRC=src
INCLUDE=include
EXECUTABLE=AsconTrailTool

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	@echo "Executing..."
	/$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
	@mkdir -p $(@D)
	@echo "Building..."
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) $^ -o $@ $(LIBRARIES) 

clean:
	@echo "Clearing..."
	-rm $(BIN)/*
	-rm *.txt