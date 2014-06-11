DEPS_LIBS := $(shell pkg-config --libs webkit2gtk-3.0)
DEPS_CFLAGS := $(shell pkg-config --cflags webkit2gtk-3.0)

CFLAGS := -ggdb

all:

gnome-books: gnome-books.o resources.o
gnome-books: CFLAGS := $(CFLAGS) $(DEPS_CFLAGS)
gnome-books: LIBS := $(LIBS) $(DEPS_LIBS)
binaries += gnome-books

all: $(binaries)

$(binaries):
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

resources.c: resources.xml
	glib-compile-resources --target=$@ --generate-source $<

clean:
	rm -rf *.o mb/*.o $(binaries) *-res.c