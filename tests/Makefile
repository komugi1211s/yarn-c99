


ifeq ($(OS), Windows_NT)
	CC = cl.exe /Zi /I"../src/"
else
	CC = clang -fsanitize=undefined -Wall --pedantic -g -I../src/
endif

test:
	$(CC) -o $@ test.c ../src/yarn_spinner.pb-c.c ../src/protobuf-c.c
	./$@
