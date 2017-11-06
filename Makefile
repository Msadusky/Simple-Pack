CC=gcc
FLAGS=-Wall -Wextra -std=c99

UNPACK=sunpack
SAMPLE=example.spack

all: $(UNPACK)

$(UNPACK): $(UNPACK).c spack.h
	$(CC) $(FLAGS) $(UNPACK).c -o $(UNPACK)

clean:
	rm -f *.o a.out $(UNPACK)

test: test.sh $(UNPACK) $(SAMPLE)
	./test.sh $(UNPACK) $(SAMPLE)
