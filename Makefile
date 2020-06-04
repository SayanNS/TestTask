output: main.o bmpreader.o shared_container.o
	g++ main.o bmpreader.o shared_container.o -o output -pthread

main.o: main.cpp
	g++ -c main.cpp

bmpreader.o: bmpreader.cpp bmpreader.h
	g++ -c bmpreader.cpp

shared_container.o: shared_container.cpp shared_container.h
	g++ -c shared_container.cpp

clean:
	rm *.o output

#BMP = tiger.bmp
BMP = chevalier.bmp

OUTPUT = output.yuv

YUV = Tiger.yuv

WIDTH = 1920

HEIGHT = 1080

run: output
	./output -b $(BMP) -y $(YUV) -w $(WIDTH) -h $(HEIGHT) -o $(OUTPUT)
