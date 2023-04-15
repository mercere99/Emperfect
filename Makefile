default: native

# Identify all directory locations
EMP_DIR   = ../Empirical
DTL_DIR   = ./dtl

TARGET := Emperfect

# Specify sets of compilation flags to use
FLAGS_version := -std=c++20
FLAGS_warn    = -Wall -Wextra -Wno-unused-function -Woverloaded-virtual -pedantic
FLAGS_include = -I$(EMP_DIR)/include/ -I${EMP_DIR}/third-party/cereal/include/ -I${DTL_DIR}/dtl
FLAGS_main    = $(FLAGS_version) $(FLAGS_warn) $(FLAGS_include) -pthread

FLAGS_QUICK  = $(FLAGS_main) -DNDEBUG
FLAGS_DEBUG  = $(FLAGS_main) -g -DEMP_TRACK_MEM
FLAGS_OPT    = $(FLAGS_main) -O3 -DNDEBUG
FLAGS_GRUMPY = $(FLAGS_main) -DNDEBUG -Wconversion -Weffc++
FLAGS_EMSCRIPTEN = --js-library $(EMP_DIR)/web/library_emp.js -s EXPORTED_FUNCTIONS="['_main', '_empCppCallback']" -s NO_EXIT_RUNTIME=1  -s TOTAL_MEMORY=67108864
FLAGS_COVERAGE = $(FLAGS_main)  -O0 -DEMP_TRACK_MEM -ftemplate-backtrace-limit=0 -fprofile-instr-generate -fcoverage-mapping -fno-inline -fno-elide-constructors

install:
	git submodule update --init --recursive

native: FLAGS := $(FLAGS_OPT)
native: $(TARGET)

debug: FLAGS := $(FLAGS_DEBUG)
debug: $(TARGET)

grumpy: FLAGS := $(FLAGS_GRUMPY)
grumpy: $(TARGET)

quick: FLAGS := $(FLAGS_QUICK)
quick: $(TARGET)

$(TARGET): src/$(TARGET).cpp
	$(CXX) $(FLAGS) src/$(TARGET).cpp -o $(TARGET)

new: clean
new: native

CXX = g++

# Debugging information
#print-%: ; @echo $*=$($*)
print-%: ; @echo '$(subst ','\'',$*=$($*))'

CLEAN_BACKUP = *~ *.dSYM
CLEAN_TEST = *.out *.o *.gcda *.gcno *.info *.gcov ./Coverage* ./temp
CLEAN_EXTRA =

CLEAN_FILES = $(CLEAN_BACKUP) $(CLEAN_TEST) $(CLEAN_EXTRA) $(TARGET)

clean:
	@echo About to remove:
	@echo $(wildcard $(CLEAN_FILES))
	@echo ----
	rm -rf $(wildcard $(CLEAN_FILES))
