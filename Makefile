
BUILD_DIR = build

# FIXME: bessere Lösung dafür finden
ifeq ($(MAKECMDGOALS),test)
SRC = $(shell find src test lib -name '*.c')
else
SRC = $(shell find src lib -name '*.c')
endif

OBJ = $(addprefix $(BUILD_DIR)/,$(SRC:%.c=%.o))

# auto generated dep files
DEP = $(OBJ:.o=.d)

CC = gcc

ifeq ($(MAKECMDGOALS),test)
CFLAGS = -Wall -Wextra -ggdb -O0
else
CFLAGS = -Wall -Wextra -s -O2
endif
CPPFLAGS = -Iinclude

ifeq ($(MAKECMDGOALS),test)
LDFLAGS = -lcriterion -lm -lpthread
else
LDFLAGS = -lspi -lprussdrvd -lm -lpthread
endif

$(OBJ):$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/program: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: all test run build clean deploy

all: run

test: run

run: build
	./$(BUILD_DIR)/program

build: $(BUILD_DIR)/program
	@echo $(SRC)

clean:
	rm -rf $(BUILD_DIR)

deploy:
	rsync -r --exclude=build/ --delete . debian@beaglebone.local:~/ppl_deployment/

# include auto generated targets
-include $(DEP)
