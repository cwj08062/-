CC = gcc

CFLAGS = -g -Wall

LDFLAGS =

LDLIBS =

PARENT_OBJS = parent.o
CHILD_OBJS = child.o

PARENT_TARGET = parent.out
CHILD_TARGET = child.out

all: $(PARENT_TARGET) $(CHILD_TARGET)

clean:    
	rm -f *.o
	rm -f $(PARENT_TARGET) $(CHILD_TARGET)

$(PARENT_TARGET): $(PARENT_OBJS)
	$(CC) -o $@ $(PARENT_OBJS) 

$(CHILD_TARGET): $(CHILD_OBJS)
	$(CC) -o $@ $(CHILD_OBJS) 


