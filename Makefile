# for "make":
CC=gcc
OUTDIR=dist
TARGET=
SOURCES=particle-life.c
LIBS=
# for "make install":
DESTDIR=
PREFIX=

# platform specific settings
ifeq ($(OS),Windows_NT)
    # Windows specific settings
	TARGET=particle-life.exe
	LIBS += pdcurses/*.o getopt/*.o
    RM=powershell Remove-Item -Recurse -Force
    MKDIR=powershell New-Item -ItemType Directory -Force
else
    # Linux specific settings
	TARGET=particle-life
	LIBS += -l ncurses -l m
    RM=rm -rf
    MKDIR=mkdir -p
	PREFIX=/usr/local
endif

all: $(OUTDIR)/$(TARGET)

$(OUTDIR):
	$(MKDIR) $(OUTDIR)

$(OUTDIR)/$(TARGET): $(OUTDIR) $(SOURCES)
	$(CC) -o $(OUTDIR)/$(TARGET) $(SOURCES) $(LIBS)

install: $(OUTDIR)/$(TARGET)
	# create install directory
	$(MKDIR) $(DESTDIR)$(PREFIX)/bin
	# copy binary
	cp $(OUTDIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin


# Phony target to clean the build artifacts
clean:
	$(RM) $(OUTDIR)

.PHONY: all clean

