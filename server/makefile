conf_dir = ../conf/
fifo_dir = ../shm_fifo/
da_dir = ../da/
analyse_dir = ../analyse/
util_dir = ../utility/
CC = gcc #Compiler name
CFLAGS = -std=c2x \
  -Wall -Wconversion -Werror -Wextra -Wpedantic -Wwrite-strings \
  -O2 \
  -I$(conf_dir) -I$(fifo_dir) -I$(da_dir) -I$(analyse_dir) -I$(util_dir)
vpath %.c $(conf_dir) $(fifo_dir) $(da_dir) $(analyse_dir) $(util_dir)
vpath %.h $(conf_dir) $(fifo_dir) $(da_dir) $(analyse_dir) $(util_dir)
objects = da.o conf.o shm_fifo.o qlunch.o analyse.o utility.o
executable = qlunch
makefile_indicator = .\#makefile\#

.PHONY: all clean

all: $(executable)

clean:
	$(RM) $(objects) $(executable)
	@$(RM) $(makefile_indicator)

$(executable): $(objects)
	$(CC) $(objects) -o $(executable)

qlunch.o: qlunch.c conf.h shm_fifo.h da.h analyse.h utility.h
conf.o: conf.c conf.h da.h
shm_fifo.o: shm_fifo.c shm_fifo.h
da.o: da.c da.h
analyse.o: analyse.c analyse.h
utility.o: utility.c utility.h conf.h

include $(makefile_indicator)

$(makefile_indicator): makefile
	@touch $@
	@$(RM) $(objects) $(executable)
