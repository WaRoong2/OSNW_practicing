TARGET = helloworld
OBJECTS = main3.o hello.o world.o
$(TARGET) : $(OBJECTS)
	gcc -o $(TARGET) $(OBJECTS)
hello.o : hello.c
	gcc -c hello.c
world.o : world.c
	gcc -c world.c
main3.o : main3.c
	gcc -c main3.c
