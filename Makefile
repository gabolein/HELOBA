CC = gcc
CFLAGS = -Wall -Wextra
CPPFLAGS = -Iinclude -DVIRTUAL
LDFLAGS = -lm -lpthread
SRC = $(shell find -name '*.c')

ifeq ($(MAKECMDGOALS),test)
SRC := $(filter-out ./src/main.c, $(SRC))
else
SRC := $(filter-out ./test/%, $(SRC))
endif

ifneq (,$(findstring -DVIRTUAL,$(CPPFLAGS)))
SRC := $(filter-out ./src/beaglebone/%, $(SRC))
else
SRC := $(filter-out ./src/virtual_transport.c, $(SRC))
endif

BUILD_DIR = build
BIN=heloba
OBJ = $(addprefix $(BUILD_DIR)/,$(SRC:%.c=%.o))
DEP = $(OBJ:.o=.d)

# include auto generated targets
-include $(DEP)

$(OBJ):$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: build run test debug release clean deploy

.DEFAULT_GOAL = debug

build: $(BUILD_DIR)/$(BIN)

run: build
	./$(BUILD_DIR)/$(BIN)

test: CFLAGS += -ggdb -O0 -fsanitize=address
test: LDFLAGS += -lcriterion -fsanitize=address
test: run

debug: CFLAGS += -ggdb -O0
debug: run

# FIXME: CPPFLAGS += -DNDEBUG sollte Debugoutput strippen, egal welches Loglevel gesetzt ist
release: CFLAGS += -s -O2
release: CPPFLAGS += -DNDEBUG
release: run

clean:
	rm -rf $(BUILD_DIR)

deploy:
	rsync -r --exclude=build/ --exclude=.vscode/ --exclude=.cache/ --delete . debian@beaglebone.local:~/ppl_deployment/
