# Target library
LIB := libfs.a
OBJS := disk.o fs.o

CC := gcc
CLFAGS := -Wall -Werror -Wextra -MMD

ifneq (%(v), 1)
Q = @
endif

all: $(LIB)

$(LIB): $(OBJS)
	@echo "CC $@"
	$(Q)ar rcs $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c $<

clean:
	@echo "CLEAN"
	$(Q)rm $(OBJS) $(LIB)