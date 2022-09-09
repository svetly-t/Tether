all:
	g++ -I src/include -L src/lib -o main main.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
mac:
	g++ -I /usr/local/include -L /usr/local/lib -o main -std=c++11 -lSDL2main -lSDL2 -lSDL2_image main.cpp 
