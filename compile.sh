#gcc -o simulator readckt.c
g++ -c command.cpp
g++ -c simulator.cpp
g++ command.o simulator.o -o simulator
