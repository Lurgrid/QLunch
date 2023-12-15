da_dir = ../da/
CC = gcc #Compiler name
CFLAGS = -std=c2x \
  -Wall -Wconversion -Werror -Wextra -Wpedantic -Wwrite-strings \
  -O2 \
  -I$(da_dir)
vpath %.c $(da_dir)
vpath %.h $(da_dir)
objects = conf.o conf_test.o da.o
executable = conf
makefile_indicator = .\#makefile\#

.PHONY: all clean

all: $(executable)

clean:
	$(RM) $(objects) $(executable)
	@$(RM) $(makefile_indicator)

$(executable): $(objects)
	$(CC) $(objects) -o $(executable)

conf_test.o: conf_test.c conf.h da.h
conf.o: conf.c conf.h da.h
da.o: da.c da.h

include $(makefile_indicator)

$(makefile_indicator): makefile
	@touch $@
	@$(RM) $(objects) $(executable)
