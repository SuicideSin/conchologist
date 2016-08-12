SRC=src
INC=include
CXX=g++
CFLAGS=-O -Wall -Wno-unused-result -I$(INC) -I$(SRC)
LIB=

MONGOOSE_SRC=$(INC)/mongoose/mongoose.c

ifeq ($(OS),Windows_NT)
	LIB+=-lWs2_32
endif

ifeq ($(shell uname),Darwin)
	CFLAGS+=-Wno-deprecated-register
endif

all: handler

handler: $(SRC)/main.cpp $(SRC)/cc_handler.cpp $(SRC)/web_handler.cpp $(MONGOOSE_SRC)
	$(CXX) $(CFLAGS) $^ $(LIB) -o $@

clean:
	-rm -f handler handler.exe