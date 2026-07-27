CXX := g++

SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

TARGET := LiF_SC

LDFLAGS := -L/usr/lib/arm-linux-gnueabihf
LDLIBS := 					\
	-lpcan					\
	-l:libmysqlcppconn.so 	

CPPFLAGS := -I$(INC_DIR)
CXXFLAGS := -Wall -Wextra -g

# reference: https://stackoverflow.com/a/3774731
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)