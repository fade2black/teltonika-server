CC = gcc
INCDIR = include
OBJDIR = objs

CFLAGS = -Wall `pkg-config --cflags glib-2.0`
LDLIBS = `pkg-config --libs glib-2.0` -levent

DEPS = $(addprefix $(INCDIR)/, hdrs.h global_consts.h)
OBJS = $(addprefix $(OBJDIR)/, error_functions.o get_num.o slots_mng.o logger.o)

EXECUTABLE = hello

$(EXECUTABLE) : $(DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(EXECUTABLE).c $(OBJS) $(LDLIBS)

$(OBJDIR)/error_functions.o : hdrs.h error_functions.*
	$(CC) -Wall -c error_functions.c

$(OBJDIR)/get_num.o : hdrs.h get_num.*
	$(CC) -Wall -c get_num.c



.PHONY : clean
clean :
	rm -f $(OBJDIR)/*.o $(EXECUTABLE)
