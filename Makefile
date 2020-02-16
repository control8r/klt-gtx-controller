CC ?= clang
CFLAGS += -g -Wall -Wpedantic -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer

controller: controller.c controller.h config.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) controller
