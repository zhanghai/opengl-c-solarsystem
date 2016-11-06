CC=clang
CCFLAGS=-std=c11 -Wall -Werror

all: main.o tga.o
	${CC} ${CCFLAGS} -lm -lGL -lGLU -lglut -o solar_system main.o tga.o

main.o: main.c tga.h
	${CC} ${CCFLAGS} -c main.c

tga.o: tga.c
	${CC} ${CCFLAGS} -c tga.c

test: all
	./solar_system

clean:
	rm -f solar_system main.o tga.o
