build :
	gcc main.c -o image_viewer.exe -lSDL3 -mwindows

run : build
	./image_viewer.exe
	
