CC := gcc

#Folders
SRCDIR := src
BUILDDIR := build
TARGETDIR := bin

#Targets
EXECUTABLE := ttserv
TARGET := $(TARGETDIR)/$(EXECUTABLE)
INSTALLBINDIR := /usr/local/bin

SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name "*.$(SRCEXT)")
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

#Folder lists
#INCDIRS  := $(shell find include/**/* -name '*.h' -exec dirname {} \; | sort | uniq)
INCLIST := $(patsubst include/%,-I include/%,$(INCDIRS))
BUILDLIST := $(patsubst include/%,$(BUILDDIR)/%,$(INCDIRS))

CFLAGS := -Wall `pkg-config --cflags glib-2.0`
LDLIBS := `pkg-config --libs glib-2.0` -levent
INC := -I include -I /usr/local/include


$(TARGET): $(OBJECTS)
	@mkdir -p $(TARGETDIR)
	@echo " Linking..."
	@echo "  Linking $(TARGET)"; $(CC) $^ -o $(TARGET) $(LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDLIST)
	@echo "Compiling $<..."; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning $(TARGET)..."; rm -r $(BUILDDIR) $(TARGET)

install:
	@echo "Installing $(EXECUTABLE)..."; cp $(TARGET) $(INSTALLBINDIR)

distclean:
	@echo "Removing $(EXECUTABLE)"; rm $(INSTALLBINDIR)/$(EXECUTABLE)

.PHONY: clean
