CXX := g++

SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

TARGET := executable

LDFLAGS := -L/usr/lib/arm-linux-gnueabihf
LDLIBS := -l:libmysqlcppconn.so -lpcan

CPPFLAGS := -I$(INC_DIR)
CXXFLAGS := -Wall -Wextra -g

SOURCES := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/mainFunctions.cpp \
	$(SRC_DIR)/pcanFunctions.cpp \
	$(SRC_DIR)/databaseFunctions.cpp

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