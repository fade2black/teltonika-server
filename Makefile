CC := gcc

#Folders
SRCDIR := src
BUILDDIR := build
TARGETDIR := bin

#Targets
EXECUTABLE := ttserv
TARGET := $(TARGETDIR)/$(EXECUTABLE)
INSTALLBINDIR := /usr/local/bin

INCDIR := include
INCFILES := $(shell find $(INCDIR) -type f -name '*.h')
OBJECTS := $(patsubst $(INCDIR)/%,$(BUILDDIR)/%,$(INCFILES:.h=.o))

CFLAGS := -Wall `pkg-config --cflags glib-2.0`
LDLIBS := `pkg-config --libs glib-2.0` -levent
INC := -I $(INCDIR) -I /usr/local/include


$(TARGET): $(OBJECTS)
	@mkdir -p $(TARGETDIR)
	@echo " Linking..."
	@echo "  Linking $(TARGET)"; $(CC) $^ -o $(TARGET) $(LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	@echo "Compiling $<..."; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning $(TARGET)..."; rm -r $(BUILDDIR) $(TARGET)

install:
	@echo "Installing $(EXECUTABLE)..."; cp $(TARGET) $(INSTALLBINDIR)

distclean:
	@echo "Removing $(EXECUTABLE)"; rm $(INSTALLBINDIR)/$(EXECUTABLE)

.PHONY: clean
