CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS= -g -lpthread
SRC_FILES=main.c

.PHONY: all clean run

all: main

main: $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

main-thread-check: $(SRC_FILES)
	clang $(CFLAGS) -o $@ $^ $(LDFLAGS) -fsanitize=thread #GePeTo dice que clang mejor para fsanitize=thread XD
	#valgrind --tool=helgrind ./main ha funcionado con la build normal, con esta no reporta cunado paro el proceso con enter

clean:
	rm -f main
	rm -f main-thread-check

run:
	./main

run-thread-check:
	#echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
	./main-thread-check
	#echo 2 | sudo tee /proc/sys/kernel/randomize_va_space  # Re-enable ASLR
