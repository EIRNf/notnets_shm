# tool macros
CXX := clang-17
CXXFLAGS := -pthread -ggdb  #   -lstdc++  #-Wall -Wextra -Werror -lstdc++ #-fPIC -shared
#-fsanitize=address -fPIE -pie -g -fPIC -g -fsanitize=thread -fno-omit-frame-pointer
DBGFLAGS := -g 
COBJFLAGS := $(CXXFLAGS) -Wall -Wextra -Werror  
LDFLAGS  := 

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
LIB_PATH := lib
#TEST_PATH := test
DBG_PATH := debug

# compile macros
TARGET_NAME := main # FILL: target name
LIB_NAME := notnets
ifeq ($(OS),Windows_NT)
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
endif
TARGET := $(BIN_PATH)/$(TARGET_NAME)
# LIB := $(LIB_PATH)/$(LIB_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))


default: makedir all

# non-phony targets
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CXX) $(COBJFLAGS) -o $@ $<

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CXX) $(COBJFLAGS) $(DBGFLAGS) -o $@ $<

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) $(OBJ_DEBUG) -o $@

#gcc -shared bin/shared/add.o bin/shared/answer.o -o bin/shared/libtq84.so
# $(LIB): $(OBJ)
# 	$(CXX) $(OBJ) -fPIC -shared -o $(addsuffix .so,$@) $(CXXFLAGS)  $(LDFLAGS)

# clean files list
DISTCLEAN_LIST := $(OBJ) \
                  $(OBJ_DEBUG)
CLEAN_LIST := $(TARGET) \
			  $(TARGET_DEBUG) \
			  $(DISTCLEAN_LIST) \
			#   $(LIB)




# phony rules
.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH) 

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug: $(TARGET_DEBUG)

.PHONY: test
test: 
	(cd tests;./runner.sh "$(CXX) $(COBJFLAGS)")

.PHONY: bench
bench: 
	(cd bench;./runner.sh "$(CXX) $(COBJFLAGS)")
	
# .PHONY: lib
# lib: $(LIB)
# 	@mkdir -p $(LIB_PATH)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

.PHONY: distclean
distclean:
	@echo CLEAN $(DISTCLEAN_LIST)
	@rm -f $(DISTCLEAN_LIST)
