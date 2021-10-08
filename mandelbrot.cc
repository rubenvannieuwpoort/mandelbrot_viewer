#include <math.h>
#include <thread>
#include <barrier>
#include <iostream>
#include <vector>
#include <complex>
#include <algorithm>
#include <SDL2/SDL.h>
#include <immintrin.h>

Uint32 *framebuffer;

int width = 640;
int height = 480;
int total_pixels = width * height;

double cx =-0.5;
double cy = 0.0;
double scale = 1 / 256.0;

int iterations = 100;

int main() {
	if (SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << "SDL initialization failed\n";
		return 1;
	}
	
	SDL_Window *window = SDL_CreateWindow("Mandelbrot viewer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, width, height);
	
	framebuffer = new Uint32[total_pixels];
	
	size_t num_threads = std::thread::hardware_concurrency();
	if (num_threads == 0)
		num_threads = 2;
	
	std::cout << "Drawing Mandelbrot set with " << num_threads << " threads.\n";
	
	std::barrier start_barrier(num_threads), end_barrier(num_threads);
	
	bool quit = false;
	auto work = [&](int n) {
		bool fullscreen = false;
		while (!quit) {
			int rows_per_thread = height / num_threads;
			int start_height = n * rows_per_thread, end_height = start_height + rows_per_thread;
			int p = start_height * width;
			for (int y = start_height; y < end_height; y++) {
				for (int x = 0; x < width; x++, p++) {
				
					// drawing stuff
					uint8_t r, g, b;
					double xx = cx + (x - width / 2.0) * scale;
					double yy = cy + (y - height / 2.0) * scale;
					std::complex<double> c(xx, yy);
					std::complex<double> z(0, 0);
					
					int count = 0;
					double threshold = 16;
					while (count < iterations) {
						z = z * z + c;
						if (imag(z) * imag(z) + real(z) * real(z) > threshold)
							break;
						count++;
					}
					
					if (abs(z) > 2.0) {
						double frac_count = count - log2(log2(abs(z)) / 4.0);
						double r_intensity = 0.5 + 0.5 * cos(3.0 + frac_count * 0.15);
						double g_intensity = 0.5 + 0.5 * cos(3.0 + frac_count * 0.15 + 0.6);
						double b_intensity = 0.5 + 0.5 * cos(3.0 + frac_count * 0.15 + 1.0);
						
						r = std::clamp(int(r_intensity * 256), 0, 255);
						g = std::clamp(int(g_intensity * 256), 0, 255);
						b = std::clamp(int(b_intensity * 256), 0, 255);
						
						framebuffer[p] = (r << 16) + (g << 8) + b;
					}
					else {
						framebuffer[p] = 0;
					}
				}
			}
			
			end_barrier.arrive_and_wait();
			
			if (n == 0) {
				SDL_UpdateTexture(texture, NULL, framebuffer, width * sizeof(Uint32));
				
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, NULL);
				SDL_RenderPresent(renderer);
				
				bool updated = false;
				while (!updated && !quit) {
					
					SDL_Event event;
					while (SDL_PollEvent(&event)) {
						if (event.type == SDL_QUIT) {
							quit = true;
							break;
						}
						else if (event.type == SDL_KEYDOWN) {
							if (event.key.keysym.sym == SDLK_ESCAPE) {
								quit = true;
								break;
							}
							else if (event.key.keysym.sym = 'f') {
								fullscreen = !fullscreen;
								if (fullscreen)
									SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
								else
									SDL_SetWindowFullscreen(window, 0);
							}
						}
						else if (event.type == SDL_WINDOWEVENT) {
							if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
								int window_width = event.window.data1;
								int window_height = event.window.data2;
								
								width = (window_width + 7) / 8 * 8;
								height = (window_height + num_threads - 1) / num_threads * num_threads;
								
								int new_total_pixels = width * height;
								if (new_total_pixels > total_pixels) {
									delete[] framebuffer;
									total_pixels = new_total_pixels;
									framebuffer = new Uint32[total_pixels];
								}
								SDL_DestroyTexture(texture);
								texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
									SDL_TEXTUREACCESS_STREAMING, width, height);
								updated = true;
							}
						}
						else if (event.type == SDL_MOUSEWHEEL) {
							if (event.wheel.y) {
								int mx, my;
								SDL_GetMouseState(&mx, &my);
								
								double new_scale = scale * pow(.9, event.wheel.y);
								cx += (scale - new_scale) * (mx - width / 2.0);
								cy += (scale - new_scale) * (my - height / 2.0);
								scale = new_scale;
								updated = true;
							}
						}
					}
				}
			}
			
			start_barrier.arrive_and_wait();
		}
	};
	
	std::vector<std::thread> threads;
	for (int i = 1; i < num_threads; i++)
		threads.emplace_back(work, i);
	
	work(0);
	
	for (int i = 0; i < num_threads - 1; i++)
		threads[i].join();
	
	delete[] framebuffer;
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	
	SDL_Quit();
	
	return 0;
}
