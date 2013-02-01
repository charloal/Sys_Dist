all: makedist

makedist: main.o parser.o Node.o  Machine.o
	mpic++ -Wextra -Wall -g  Node.o parser.o main.o Machine.o -o makedist
main.o: main.cpp
	mpic++  -Wall -g -c main.cpp
parser.o: parser.cpp
	mpic++ -Wextra -Wall -g -c parser.cpp
Node.o: Node.cpp Node.h
	mpic++ -Wextra -Wall -g -c Node.cpp
Machine.o: Machine.cpp Machine.h
	mpic++ -Wextra -Wall -g -c Machine.cpp
clean:
	rm -f *.o
distclean: clean
	rm -f parser.o main.o makedist
