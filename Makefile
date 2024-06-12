all:
	g++ -o scrollgl main.c glad.c -I./include -L./lib-arm64 -framework OpenGL -lglfw3 -framework Cocoa -framework IOKit
