# Alvo principal
all: build_dir compile link

build_dir:
		if not exist build mkdir build

compile:
all:
		if not exist build mkdir build
		g++ -Wall -Wextra -std=c++17 -I../include -c ../src/RedisCommandHandler.cpp -o build/RedisCommandHandler.o
		g++ -Wall -Wextra -std=c++17 -I../include -c ../src/RedisServer.cpp -o build/RedisServer.o
		g++ -Wall -Wextra -std=c++17 -I../include -c ../src/main.cpp -o build/main.o
		g++ -Wall -Wextra -std=c++17 build/RedisCommandHandler.o build/RedisServer.o build/main.o -o ../my_redis_cli

link:
		g++ -Wall -Wextra -std=c++17 build/CommandHandler.o build/ResponseParser.o build/CLI.o build/RedisClient.o build/main.o -o ../my_redis_cli