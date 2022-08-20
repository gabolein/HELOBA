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
LDFLAGS += -lspi -lprussdrvd
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

.PHONY: build debug release simulation run test clean deploy

.DEFAULT_GOAL = debug

build: $(BUILD_DIR)/$(BIN)

debug: CFLAGS += -ggdb -O0 -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: build

release: CFLAGS += -s -O2
release: CPPFLAGS += -DNDEBUG
release: build

simulation: CFLAGS += -s -O2
simulation: build

run: build
	./$(BUILD_DIR)/$(BIN)

test: CFLAGS += -ggdb -O0 -fsanitize=address
test: LDFLAGS += -lcriterion -fsanitize=address
test: run

clean:
	rm -rf $(BUILD_DIR)

deploy:
	rsync -r --exclude=build/ --exclude=.vscode/ --exclude=.cache/ --delete . debian@beaglebone.local:~/ppl_deployment/

# for testing on multiple Beaglebones. IP Adresses to be replaced
maxiDeploy:
	rsync -r --exclude=build/ --exclude=.vscode/ --exclude=.cache/ --delete . debian@192.168.178.121:~/ppl_deployment/
	rsync -r --exclude=build/ --exclude=.vscode/ --exclude=.cache/ --delete . debian@192.168.178.143:~/ppl_deployment/
