CFLAGS = -g -Wall -pthread
CC = gcc

# Arguments for routers and ne
NE_HOSTNAME = localhost
NE_UDP_PORT = 2062
R0_UDP_PORT = 3062
R1_UDP_PORT = 4062
R2_UDP_PORT = 5062
R3_UDP_PORT = 6062
R4_UDP_PORT = 7062
CONFIG_FILE =  4_routers.conf

# change based on type of router to be built
# value can be either DISTVECTOR or PATHVECTOR
ROUTERMODE = PATHVECTOR

# if DEBUG is 1, debugging messages are printed
DEBUG = 1

# Check which OS
OS := $(shell uname)
ifeq ($(OS), SunOS)
	SOCKETLIB = -lsocket
endif

all : router

endian.o   :   ne.h endian.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c endian.c

routingtable.o   :   ne.h routingtable.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c routingtable.c
	
router  :   endian.o routingtable.o router.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) endian.o routingtable.o router.c -o router -lnsl $(SOCKETLIB)

unit-test  : routingtable.o unit-test.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) routingtable.o unit-test.c -o unit-test -lnsl $(SOCKETLIB)

.PHONY: submit-checkpoint
submit-checkpoint:
	zip lab2checkpoint.mann53 routingtable.c router.c


udp_client: udp_client.c routingtable.c endian.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) udp_client.c routingtable.c endian.c -o udp_client

.PHONY: run_udp_client
run_udp_client:
	./udp_client 0 localhost $(R1_UDP_PORT) 1

.PHONY: test
test:
	./unit-test

.PHONY: start_ne
start_ne: 
	./ne $(NE_UDP_PORT) $(CONFIG_FILE)

.PHONY: golden_run_0
golden_run_0: 
	./golden_router 0 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R0_UDP_PORT)

.PHONY: golden_run_1
golden_run_1: 
	./golden_router 1 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R1_UDP_PORT)

.PHONY: golden_run_2
golden_run_2: 
	./golden_router 2 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R2_UDP_PORT)

.PHONY: golden_run_3
golden_run_3: 
	./golden_router 3 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R3_UDP_PORT)

.PHONY: golden_run_4
golden_run_4: 
	./golden_router 4 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R4_UDP_PORT)

.PHONY: run_0
run_0: router
	./router 0 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R0_UDP_PORT)

.PHONY: run_1
run_1: router
	./router 1 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R1_UDP_PORT)

.PHONY: run_2
run_2: router
	./router 2 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R2_UDP_PORT)

.PHONY: run_3
run_3: router
	./router 3 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R3_UDP_PORT)

.PHONY: run_4
run_4: router
	./router 4 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R4_UDP_PORT)

.PHONY: debug
debug:
	gdb --args router 0 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R1_UDP_PORT)

.PHONY: memory
memory:
	valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-reachable=yes ./router 0 $(NE_HOSTNAME) $(NE_UDP_PORT) $(R0_UDP_PORT)

.PHONY: submit-final
submit-final:
	zip lab2final.mann53 routingtable.c router.c

clean :
	rm -f *.o
	rm -f router
	rm -f unit-test
	rm -f lab2checkpoint.mann53
	rm -f *.log
	rm -f lab2final.mann53
	rm -f udp_server
