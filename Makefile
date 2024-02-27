# tool macros
CC := clang-17
CCFLAGS :=  -pthread -ggdb#   -lstdc++  #-Wall -Wextra -Werror -lstdc++ #-fPIC -shared
#-fsanitize=address -fPIE -pie -g -fPIC -g -fsanitize=thread -fno-omit-frame-pointer
COBJFLAGS := -Wall -Wextra -Werror $(CCFLAGS) 
LDFLAGS  := -shared

# path macros
OBJ_PATH := obj
SRC_PATH := src
LIB_PATH := lib
# compile macros
LIB_NAME := notnets.so
LIB := $(LIB_PATH)/$(LIB_NAME)

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

default: makedir lib

# non-phony targets
$(TARGET): $(OBJ)
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $^

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c*
	$(CC) $(COBJFLAGS) -o $@ $<

#gcc -shared bin/shared/add.o bin/shared/answer.o -o bin/shared/libtq84.so
$(LIB): $(OBJ)
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $^

# clean files list
DISTCLEAN_LIST := $(OBJ) 

CLEAN_LIST := $(DISTCLEAN_LIST) \
			  $(LIB) 


.PHONY: makedir
makedir:
	@mkdir -p $(OBJ_PATH) $(LIB_PATH)

.PHONY: lib
lib: $(LIB)

.PHONY: test
test: 
	(cd tests;./runner.sh "$(CC) $(COBJFLAGS)")

.PHONY: bench
bench: 
	(cd bench;./runner.sh "$(CC) $(COBJFLAGS)")
	
.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)
	@echo CLEAN $(OBJ_PATH) $(LIB_PATH)
	@rmdir $(OBJ_PATH) $(LIB_PATH)


.PHONY: distclean
distclean:
	@echo CLEAN $(DISTCLEAN_LIST)
	@rm -f $(DISTCLEAN_LIST)
