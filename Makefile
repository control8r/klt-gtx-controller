CC ?= clang
CFLAGS += -g -Wall -Wpedantic -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer

controller: controller.c controller.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) controller
