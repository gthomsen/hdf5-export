LIBRARY = hdf5_export.so

CC = gcc

CFLAGS = -Wall -W -Werror -fPIC -ggdb3
LFLAGS = -lhdf5_hl -lhdf5 -fPIC -shared -Wl,-soname,hdf5_export.so -ggdb3

all: $(LIBRARY)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIBRARY): hdf5_export.o
	$(CC) -o $@ $(LFLAGS) $^
