PREFIX=.

override CFLAGS:=-MMD -Wall -O3 $(CFLAGS)
override LDFLAGS:=$(LDFLAGS)

sources=          \
	src/main.c        \
	src/tracee.c      \
	src/options.c     \
	src/output.c      \
	src/output-tree.c \
	src/output-json.c \

objects=$(sources:%.c=%.o)
depends=$(sources:%.c=%.d)
program=process-tree

all: $(program)

$(program): $(objects)
	$(CC) $(LDFLAGS) -o $(@) $(^)

-include $(depends)

.c.o:
	$(CC) $(CFLAGS) -o $(@) -c $(<)

clean:
	rm -rf $(program) **/*.o **/*.d

install:
	install -Dm755 $(program) $(PREFIX)/bin/$(program)

.PHONY: all clean install
