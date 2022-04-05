PREFIX  := /usr/local
CC      := cc
CFLAGS  := -pedantic -Wall -Wno-deprecated-declarations -Os
LDFLAGS := -lX11

TARGET := dwmblocks
SOURCE := *.c

# FreeBSD (uncomment)
#LDFLAGS += -L/usr/local/lib -I/usr/local/include
# OpenBSD (uncomment)
#LDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include

all: $(TARGET)

$(TARGET): $(SOURCE)
	${CC} ${CFLAGS} ${LDFLAGS} $(SOURCE) -o $(TARGET)

clean:
	rm -f *.o *.gch $(TARGET)

install: $(TARGET)
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f $(TARGET) ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/$(TARGET)

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/$(TARGET)

.PHONY: all clean install uninstall
