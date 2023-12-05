HEADERS = ../lsw_laser_pdu.h
OBJECTS = system_state.o lsw_laser_pdu.o main.o

default: lsw_laser_pdu_test

main.o: main.c $(HEADERS)
	gcc -c $< -o $@

%.o: ../%.c $(HEADERS)
	gcc -c $< -o $@

lsw_laser_pdu_test: $(OBJECTS)
	gcc $(OBJECTS) -o $@ -pthread

clean:
	-rm -f $(OBJECTS)
	-rm -f lsw_laser_pdu_test
