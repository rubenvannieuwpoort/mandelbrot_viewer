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

int iterations = 256;

float scale = 1 / 256.0f;
__m256 scalevec = _mm256_set1_ps(scale);
__m256 threshold = _mm256_set1_ps(4);
__m256 one = _mm256_set1_ps(1);
__m256 iter_scale = _mm256_set1_ps(1.0f / 255);
__m256 depth_scale = _mm256_set1_ps(255);

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
				for (int x = 0; x < width; x += 4, p += 8) {
					float xx = x - width / 2.0f;
					__m256 mx = _mm256_set_ps(xx + 7, xx + 6, xx + 5, xx + 4,
					                          xx + 3, xx + 2, xx + 1, xx + 0);
					mx = _mm256_add_ps(_mm256_mul_ps(mx, scalevec), _mm256_set1_ps(cx));
					__m256 my = _mm256_set1_ps(cy + (y - height / 2.0f) * scale);
					__m256 cr = mx;
					__m256 ci = my;
					__m256 zr = mx;
					__m256 zi = my;
					
					int count = 1;
					__m256 mk = _mm256_set1_ps(1);
					//double threshold = 16;
					while (++count < iterations) {
						__m256 zr2 = _mm256_mul_ps(zr, zr);
						__m256 zi2 = _mm256_mul_ps(zi, zi);
						__m256 zrzi = _mm256_mul_ps(zr, zi);
						/* zr1 = zr0 * zr0 - zi0 * zi0 + cr */
						/* zi1 = zr0 * zi0 + zr0 * zi0 + ci */
						zr = _mm256_add_ps(_mm256_sub_ps(zr2, zi2), cr);
						zi = _mm256_add_ps(_mm256_add_ps(zrzi, zrzi), ci);
						
						/* Increment k */
						zr2 = _mm256_mul_ps(zr, zr);
						zi2 = _mm256_mul_ps(zi, zi);
						__m256 mag2 = _mm256_add_ps(zr2, zi2);
						__m256 mask = _mm256_cmp_ps(mag2, threshold, _CMP_LT_OS);
						mk = _mm256_add_ps(_mm256_and_ps(mask, one), mk);
						
						/* Early bailout? */
						if (_mm256_testz_ps(mask, _mm256_set1_ps(-1)))
							break;
					}
					
					mk = _mm256_mul_ps(mk, iter_scale);
					mk = _mm256_sqrt_ps(mk);
					mk = _mm256_mul_ps(mk, depth_scale);
					__m256i pixels = _mm256_cvtps_epi32(mk);
					unsigned char *dst = (unsigned char *)&(framebuffer[y * width + x]);
					unsigned char *src = (unsigned char *)&pixels;
					for (int i = 0; i < 8; i++) {
						dst[i * 4 + 0] = 255 - src[i * 4];
						dst[i * 4 + 1] = 255 - src[i * 4];
						dst[i * 4 + 2] = 255 - src[i * 4];
					}
					
					/*if (abs(z) > 2.0) {
						double frac_count = count - log2(log2(abs(z)) / 4.0);
						double r_intensity = 0.5 + 0.5 * cos(3.0 + frac_count * 0.15);
						double g_intensity = 0.5 + 0.5 * cos(3.0 + frac_count * 0.15 + 0.6);
						double b_intensity = 0.5 + 0.5 * cos(3.0 + frac_count * 0.15 + 1.0);
						
						uint8_t r, g, b;
						r = std::clamp(int(r_intensity * 256), 0, 255);
						g = std::clamp(int(g_intensity * 256), 0, 255);
						b = std::clamp(int(b_intensity * 256), 0, 255);
						
						framebuffer[p] = (r << 16) + (g << 8) + b;
					}
					else {
						framebuffer[p] = 0;
					}*/
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
							else if (event.key.keysym.sym == 'f') {
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
								
								float new_scale = scale * pow(.9, event.wheel.y);
								cx += (scale - new_scale) * (mx - width / 2.0);
								cy += (scale - new_scale) * (my - height / 2.0);
								scale = new_scale;
								scalevec = _mm256_set1_ps(scale);
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
