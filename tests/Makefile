


ifeq ($(OS), Windows_NT)
	CC = cl.exe /Zi /I"../src/"
else
	CC = clang -g -I../src/
endif

test:
	$(CC) test.c ../src/yarn_spinner.pb-c.c ../src/protobuf-c.c
	$@
