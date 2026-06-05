# =============================================================
# MusicPlayer/Makefile
# =============================================================

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I./include
LDFLAGS  = -lpthread

SRC_DIR  = src
BIN      = music_player

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, build/%.o, $(SRCS))

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  Built: $(BIN)"

build/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p build
	@echo "  CXX $<"
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: all
	./$(BIN)

clean:
	rm -rf build/ $(BIN)
