#gcc -o simulator readckt.c
g++ -g -c command.cpp
g++ -g -c atpg.cpp
g++ -g -c simulator.cpp
g++ -g command.o atpg.o simulator.o -o simulator
rm command.o atpg.o simulator.o