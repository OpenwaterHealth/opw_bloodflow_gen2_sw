HEADERS = ../uart.h
OBJECTS = uart.o main.o

default: uart_test

uart.o: ../uart.c $(HEADERS)
	gcc -c $< -o $@

%.o: %.c $(HEADERS)
	gcc -c $< -o $@

uart_test: $(OBJECTS)
	gcc $(OBJECTS) -o $@ -pthread

clean:
	-rm -f $(OBJECTS)
	-rm -f uart_test
