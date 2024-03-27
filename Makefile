TARGET=rpcserver
SOURCES=rpcconvert.cpp rpcfile.cpp rpcio.cpp rpckeyvaluestore.cpp rpcmath.cpp rpcqueue.cpp $(TARGET).cpp rpcmain.cpp
CXXFLAGS=-std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -O2
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(SOURCES:.cpp=.d)

CXX=clang++

all: $(TARGET)

clean:
	-rm -rf $(DEPS) $(OBJECTS)

spotless: clean
	-rm -rf $(TARGET)

format:
	clang-format -i $(SOURCES) $(INCLUDES)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) -lpthread

-include $(DEPS)

.PHONY: all clean format spotless
