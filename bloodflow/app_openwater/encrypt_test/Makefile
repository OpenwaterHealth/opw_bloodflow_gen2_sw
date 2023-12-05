HEADERS = ../encrypt_data.h
OBJECTS = encrypt_data.o main.o

default: encrypt_test

encrypt_data.o: ../encrypt_data.c $(HEADERS)
	gcc -c $< -o $@

%.o: %.c $(HEADERS)
	gcc -c $< -o $@

encrypt_test: $(OBJECTS)
	gcc $(OBJECTS) -o $@ -L/usr/local/ssl/lib -lssl -lcrypto

clean:
	-rm -f $(OBJECTS)
	-rm -f encrypt_test
