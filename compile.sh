#gcc -o simulator readckt.c
g++ -g -c command.cpp
g++ -g -c simulator.cpp
g++ -g command.o simulator.o -o simulator
