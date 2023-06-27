
MAIN = vtraking
CC = gcc
CFLAGS = -Wall
OBJ1 = RealTimeTask
OBJS = $(MAIN).o $(OBJ1).o
LIBS = -lpthread -lm `allegro-config --libs`
$(MAIN): $(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(LIBS) $(CFLAGS)
$(MAIN).o: $(MAIN).c
	$(CC) -c $(MAIN).c

clean:
	rm -rf *o $(MAIN)
