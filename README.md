# Mandelbrot viewer

This is a simple Mandelbrot viewer I made to practice with C++ concurrency, SIMD, and SDL2. Only Linux is supported.

The `mandelbrot` binary uses multiple threads to draw the Mandelbrot set using double precision. It works best on CPUs with many cores. It defaults to using 100 iterations.

The `mandelbrot_simd` uses AVX2 on multiple threads. So, it needs an AVX2-capable CPU and it also benefits from more cores. It uses single precision and defaults to using 256 iterations.


## Instructions

Run `make` to generate the binaries (you might need to install the development packages for SDL2 and upgrade your compiler to a recent version that supports std::barrier).

Now, run either the `mandelbrot` or the `mandelbrot_simd` binary. Scroll to zoom in or out, press F to toggle fullscreen, and press ESC to exit.

There is currently no option to change the number of iterations. If the viewer is laggy, try resizing the window to a smaller size, this should speed up rendering.
