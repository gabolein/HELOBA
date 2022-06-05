BUILD_DIR = build
SRC = $(shell find src -name '*.c')
OBJ = $(addprefix $(BUILD_DIR)/,$(SRC:%.c=%.o))

# auto generated dep files
DEP = $(OBJ:.o=.d)

# toolchain
CC = gcc

# configuration
CFLAGS = -Wall -Wextra -g -fstack-protector-strong
CPPFLAGS = -Iinclude
LDFLAGS = -lspi -lprussdrvd -lm -lpthread

# Regeln
.PHONY: all

# default target
all: run

# include auto generated targets
-include $(DEP)

$(OBJ):$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/program: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: build run clean deploy

build: $(BUILD_DIR)/program

run: build
	./$(BUILD_DIR)/program

clean:
	rm -rf $(BUILD_DIR)

deploy:
	rsync -r --exclude=build/ --delete . debian@beaglebone.local:~/ppl_deployment/ 
