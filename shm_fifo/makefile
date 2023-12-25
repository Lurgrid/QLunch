CC = gcc #Compiler name
CFLAGS = -std=c2x \
  -Wall -Wconversion -Werror -Wextra -Wpedantic -Wwrite-strings \
  -O2
objects = shm_fifo.o shm_fifo_test.o
executable = shm_fifo
makefile_indicator = .\#makefile\#

.PHONY: all clean

all: $(executable)

clean:
	$(RM) $(objects) $(executable)
	@$(RM) $(makefile_indicator)

$(executable): $(objects)
	$(CC) $(objects) -o $(executable)

shm_fifo_test.o: shm_fifo_test.c shm_fifo.h
shm_fifo.o: shm_fifo.c shm_fifo.h

include $(makefile_indicator)

$(makefile_indicator): makefile
	@touch $@
	@$(RM) $(objects) $(executable)
