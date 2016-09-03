SRC=src
INC=include
CXX=g++
CFLAGS=-O -Wall -Wno-unused-result -I$(INC) -I$(SRC)
LIB=

HANDLER_SRC=$(SRC)/rev_handler.cpp $(SRC)/json_util.cpp $(SRC)/main.cpp $(SRC)/pack_util.cpp $(SRC)/web_handler.cpp
JSONCPP_SRC=$(INC)/jsoncpp/json_reader.cpp $(INC)/jsoncpp/json_value.cpp $(INC)/jsoncpp/json_writer.cpp
MONGOOSE_SRC=$(INC)/mongoose/mongoose.c
MSL_SRC=$(INC)/msl/crypto.cpp $(INC)/msl/string.cpp

ifeq ($(OS),Windows_NT)
	LIB+=-lWs2_32
else
	LIB+=-lcrypto
endif

ifeq ($(shell uname),Darwin)
	CFLAGS+=-Wno-deprecated-register
endif

all: conchologist

conchologist: $(HANDLER_SRC) $(JSONCPP_SRC) $(MONGOOSE_SRC) $(MSL_SRC)
	$(CXX) $(CFLAGS) $^ $(LIB) -o $@

clean:
	-rm -f conchologist conchologist.exe