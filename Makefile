CC = gcc
BUILD_DIR = build

# FIXME: bessere Lösung dafür finden
ifeq ($(MAKECMDGOALS),test)
SRC = $(shell find test src lib -name '*.c' ! -wholename src/main.c)
CFLAGS = -Wall -Wextra -ggdb -O0 -fsanitize=address
CPPFLAGS = -Iinclude -DVIRTUAL
LDFLAGS = -lcriterion -lm -lpthread -fsanitize=address
else
SRC = $(shell find src lib -name '*.c')
CFLAGS = -Wall -Wextra -ggdb -O0 -fsanitize=address
CPPFLAGS = -Iinclude -DVIRTUAL
LDFLAGS = -lm -lpthread -fsanitize=address
endif

ifneq (,$(findstring -DVIRTUAL,$(CPPFLAGS)))
SRC := $(filter-out src/beaglebone/%, $(SRC))
else
SRC := $(filter-out src/virtual_transport.c, $(SRC))
endif

OBJ = $(addprefix $(BUILD_DIR)/,$(SRC:%.c=%.o))
DEP = $(OBJ:.o=.d)

$(OBJ):$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/program: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: test run build clean deploy

.DEFAULT_GOAL = run

test: run

run: build
	./$(BUILD_DIR)/program

build: $(BUILD_DIR)/program

clean:
	rm -rf $(BUILD_DIR)

deploy:
	rsync -r --exclude=build/ --delete . debian@beaglebone.local:~/ppl_deployment/

# include auto generated targets
-include $(DEP)
