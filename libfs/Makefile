# Target library
LIB := libfs.a
OBJS := disk.o fs.o

CC := gcc
CLFAGS := -Wall -Werror -Wextra -MMD

ifneq (%(v), 1)
Q = @
endif

all: $(LIB)

#DEPS = $(patsubst %.o, %.d, $(OBJS))
#-include $(DEPS)

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c $<

%.a: $(OBJS)
	@echo "CC $@"
	$(Q)ar rcs $@ $^

clean:
	@echo "CLEAN"
	$(Q)rm $(OBJS) $(LIB)