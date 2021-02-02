CC = g++
objects = test_derive.o
test_derive: $(objects)
	$(CC) -o test_derive $(objects)

test_derive.o: test_derive.cpp test_derive.hpp


.PHONY: clean
clean:
	-rm test_derive $(objects)
