CC = g++
CFLAGS = -std=c++11 -Wall
LDFLAGS = -lglfw -ldl -lm -lGL -lIL

C_CC = gcc
C_CFLAGS = -std=c99 -Wall

SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)
CSOURCES = glxw.c
COBJECTS = $(CSOURCES:.c=.co)

VPATH = source
OUTPUT = particles

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS) $(COBJECTS)
	$(CC) $(OBJECTS) $(COBJECTS) $(LDFLAGS) -o $(OUTPUT)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.co: %.c
	$(C_CC) $(C_CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.co $(OUTPUT)
