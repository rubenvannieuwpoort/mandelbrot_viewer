.PHONY: all clean

BINARIES = mandelbrot mandelbrot_simd

all: $(BINARIES)

mandelbrot: mandelbrot.cc
	g++ mandelbrot.cc -o mandelbrot -g -std=c++20 -lSDL2 -pthread -O3

mandelbrot_simd: mandelbrot_simd.cc
	g++ mandelbrot_simd.cc -o mandelbrot_simd -g -std=c++20 -lSDL2 -pthread -O3 -mavx

clean:
	rm $(BINARIES)
