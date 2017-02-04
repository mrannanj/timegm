TARG := timegm-test
CFLAGS := -O0 -fno-omit-frame-pointer -ggdb3 -fstack-protector-all $(CFLAGS)
CFLAGS += -D_DEFAULT_SOURCE -std=c89

W := -Wextra -Wall -pedantic -Wno-unused-parameter -Wno-unused-result
W += -Wno-unused

MAKEFLAGS := -j4

.PHONY: all clean

all: $(TARG)

timegm-test: timegm-test.o timegm.o
	$(CC) $(W) $(CFLAGS) $^ -o $@ -lm

%.o: %.c
	$(CC) $(W) $(CFLAGS) -c $^ -lm

clean:
	$(RM) $(TARG) timegm-test.o timegm.o
