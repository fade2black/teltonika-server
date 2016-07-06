CC := gcc

#Folders
SRCDIR := src
INCDIR := include
BUILDDIR := build
TARGETDIR := bin

#Targets
EXECUTABLE := ttserv
TARGET := $(TARGETDIR)/$(EXECUTABLE)
INSTALLBINDIR := /usr/local/bin

SRCFILES := $(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SRCFILES:.c=.o))

CFLAGS := -Wall `pkg-config --cflags glib-2.0`
LDLIBS := `pkg-config --libs glib-2.0` -levent -pthread -lpq
INC := -I $(INCDIR) -I /usr/local/include -I `pg_config --includedir`


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
