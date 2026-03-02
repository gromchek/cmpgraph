CXX = g++
CXXFLAGS_MAIN = -O3 -Wall -Wextra
CXXFLAGS_TEST = -O3

MAIN_APP = cmpgraph
TEST_APP = test_transform

SRC_DIR = src

MAIN_SRC = $(SRC_DIR)/main.cpp $(SRC_DIR)/CliArgs.cpp $(SRC_DIR)/StringTransformer.cpp $(SRC_DIR)/FunctionMatcher.cpp
TEST_SRC = $(SRC_DIR)/test_transform.cpp $(SRC_DIR)/StringTransformer.cpp

GREP_FILTER = grep -vF -e '__jump' -e 'FMOD' -e 'BSN::' -e 'std::' -e 'FUN_' -e ' _' -e 'ProtocolHard' -e 'Blizzard' -e 'Battlenet' -e 'CAnimKitManager'

SRCS := $(shell find $(SRC_DIR) -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h")

.PHONY: all clean test run_410 run_501 run_601 run_053 run_all

all: $(MAIN_APP) $(TEST_APP)

$(MAIN_APP): $(MAIN_SRC)
	$(CXX) $(MAIN_SRC) $(CXXFLAGS_MAIN) -o $(MAIN_APP)

$(TEST_APP): $(TEST_SRC)
	$(CXX) $(TEST_SRC) $(CXXFLAGS_TEST) -o $(TEST_APP)

test: $(TEST_APP)
	./$(TEST_APP)

run_410: $(MAIN_APP)
	./$(MAIN_APP) --base 335_call_graph.json --ref 410_call_graph.json -o 410result.txt

run_501: $(MAIN_APP)
	./$(MAIN_APP) --base 335_call_graph.json --ref 501_call_graph.json -o 501result.txt

run_601: $(MAIN_APP)
	./$(MAIN_APP) --base 335_call_graph.json --ref 601_call_graph.json -o 601result.txt

run_053: $(MAIN_APP)
	./$(MAIN_APP) --base 335_call_graph.json --ref 053_call_graph.json -o 053result.txt

run_all: run_410 run_501 run_601 run_053

to_proc:
	$(GREP_FILTER) 410result.txt > 410_to_process.txt
	$(GREP_FILTER) 501result.txt > 501_to_process.txt
	$(GREP_FILTER) 601result.txt > 601_to_process.txt
	$(GREP_FILTER) 053result.txt > 053_to_process.txt

format:
	clang-format -i $(SRCS)

clean:
	rm -f $(MAIN_APP) $(MAIN_APP).exe $(TEST_APP) $(TEST_APP).exe *result.txt