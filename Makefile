SRC=src
INC=include
CXX=g++
CFLAGS=-O -Wall -Wno-unused-result -I$(INC) -I$(SRC)
LIB=

HANDLER_SRC=$(SRC)/cc_handler.cpp $(SRC)/json_util.cpp $(SRC)/main.cpp $(SRC)/web_handler.cpp
JSONCPP_SRC=$(INC)/jsoncpp/json_reader.cpp $(INC)/jsoncpp/json_value.cpp $(INC)/jsoncpp/json_writer.cpp
MONGOOSE_SRC=$(INC)/mongoose/mongoose.c

ifeq ($(OS),Windows_NT)
	LIB+=-lWs2_32
endif

ifeq ($(shell uname),Darwin)
	CFLAGS+=-Wno-deprecated-register
endif

all: handler

handler: $(HANDLER_SRC) $(JSONCPP_SRC) $(MONGOOSE_SRC)
	$(CXX) $(CFLAGS) $^ $(LIB) -o $@

clean:
	-rm -f handler handler.exe