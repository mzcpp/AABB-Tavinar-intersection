#include "Game.hpp"
#include "Constants.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <cstdint>
#include <iostream>
#include <cmath>

Game::Game() : 
	initialized_(false), 
	running_(false), 
	cell_size_(32), 
	cells_width_(constants::screen_width / cell_size_), 
	cells_height_(constants::screen_height / cell_size_), 
	mouse_left_pressed_(false), 
	mouse_right_pressed_(false), 
	setting_walls_(true), 
	render_line_(false)
{
	initialized_ = Initialize();

	board_.resize(cells_width_ * cells_height_);

	for (int y = 0; y < cells_height_; ++y)
	{
		for (int x = 0; x < cells_width_; ++x)
		{
			const int index = y * cells_width_ + x;

			board_[index].rect_.x = x * cell_size_;
			board_[index].rect_.y = y * cell_size_;
			board_[index].rect_.w = cell_size_;
			board_[index].rect_.h = board_[index].rect_.w;
			board_[index].is_wall_ = false;
			board_[index].highlighted_ = false;
		}
	}

	const int box_size = 10;

	player_.box_.x = (constants::screen_width * 1 / 3) - (box_size / 2);
	player_.box_.y = (constants::screen_height / 2) - (box_size / 2);
	player_.box_.w = box_size;
	player_.box_.h = box_size;
	player_.vx_ = 0;
	player_.vy_ = 0;

	mouse_box_.x = (constants::screen_width * 2 / 3) - (box_size / 2);
	mouse_box_.y = (constants::screen_height / 2) - (box_size / 2);
	mouse_box_.w = box_size;
	mouse_box_.h = box_size;

	mouse_position_ = { 0, 0 };
}

Game::~Game()
{
	Finalize();
}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
	{
		printf("%s\n", "Warning: Texture filtering is not enabled!");
	}

	window_ = SDL_CreateWindow(constants::game_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, constants::screen_width, constants::screen_height, SDL_WINDOW_SHOWN);

	if (window_ == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	if (renderer_ == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	constexpr int img_flags = IMG_INIT_PNG;

	if (!(IMG_Init(img_flags) & img_flags))
	{
		printf("SDL_image could not be initialized! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	return true;
}

void Game::Finalize()
{
	SDL_DestroyWindow(window_);
	window_ = nullptr;
	
	SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	SDL_Quit();
	IMG_Quit();
}

void Game::Run()
{
	if (!initialized_)
	{
		return;
	}

	running_ = true;

	constexpr double ms = 1.0 / 60.0;
	std::uint64_t last_time = SDL_GetPerformanceCounter();
	long double delta = 0.0;

	double timer = SDL_GetTicks();

	int frames = 0;
	int ticks = 0;

	while (running_)
	{
		const std::uint64_t now = SDL_GetPerformanceCounter();
		const long double elapsed = static_cast<long double>(now - last_time) / static_cast<long double>(SDL_GetPerformanceFrequency());

		last_time = now;
		delta += elapsed;

		HandleEvents();

		while (delta >= ms)
		{
			Tick();
			delta -= ms;
			++ticks;
		}

		//printf("%Lf\n", delta / ms);
		Render();
		++frames;

		if (SDL_GetTicks() - timer > 1000.0)
		{
			timer += 1000.0;
			//printf("Frames: %d, Ticks: %d\n", frames, ticks);
			frames = 0;
			ticks = 0;
		}
	}
}

void Game::HandleEvents()
{
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0)
	{
		SDL_GetMouseState(&mouse_position_.x, &mouse_position_.y);

		if (e.type == SDL_QUIT)
		{
			running_ = false;
			return;
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
			{
				render_line_ = true;
				mouse_left_pressed_ = true;
			}
			else if (e.button.button == SDL_BUTTON_RIGHT)
			{
				mouse_right_pressed_ = true;
				const int index = (mouse_position_.y / cell_size_) * cells_width_ + (mouse_position_.x / cell_size_);
				
				if (index >= 0 && index < board_.size())
				{
					setting_walls_ = !board_[index].is_wall_;
					board_[index].is_wall_ = !board_[index].is_wall_;
				}
			}
		}
		else if (e.type == SDL_MOUSEBUTTONUP)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
			{
				render_line_ = false;
				mouse_left_pressed_ = false;

				for (Cell& cell : board_)
				{
					if (!cell.is_wall_)
					{
						continue;
					}
				
					cell.highlighted_ = false;
				}
			}
			else if (e.button.button == SDL_BUTTON_RIGHT)
			{
				mouse_right_pressed_ = false;
			}
		}
		
		if (e.type == SDL_MOUSEMOTION)
		{
			mouse_box_.x = mouse_position_.x - mouse_box_.w / 2;
			mouse_box_.y = mouse_position_.y - mouse_box_.w / 2;

			if (mouse_right_pressed_)
			{
				const int index = (mouse_position_.y / cell_size_) * cells_width_ + (mouse_position_.x / cell_size_);
				
				if (index >= 0 && index < board_.size())
				{
					board_[index].is_wall_ = setting_walls_;
				}
			}
		}

		int speed = 5;

		if (e.type == SDL_KEYDOWN)
		{
			if (e.key.keysym.sym == SDLK_w)
			{
				player_.vy_ = -speed;
			}

			if (e.key.keysym.sym == SDLK_a)
			{
				player_.vx_ = -speed;
			}

			if (e.key.keysym.sym == SDLK_s)
			{
				player_.vy_ = speed;
			}

			if (e.key.keysym.sym == SDLK_d)
			{
				player_.vx_ = speed;
			}
		}
		else if (e.type == SDL_KEYUP)
		{
			if (e.key.keysym.sym == SDLK_w)
			{
				player_.vy_ = 0;
			}

			if (e.key.keysym.sym == SDLK_a)
			{
				player_.vx_ = 0;
			}

			if (e.key.keysym.sym == SDLK_s)
			{
				player_.vy_ = 0;
			}

			if (e.key.keysym.sym == SDLK_d)
			{
				player_.vx_ = 0;
			}
		}
	}
}

void Game::Tick()
{
	player_.box_.x += player_.vx_;
	player_.box_.y += player_.vy_;

	if (mouse_left_pressed_)
	{
		Ray ray;

		// origin: center of player
		ray.origin_.x = static_cast<float>(player_.box_.x + (player_.box_.w / 2.0f));
		ray.origin_.y = static_cast<float>(player_.box_.y + (player_.box_.h / 2.0f));

		// target: center of mouse box
		float tx = static_cast<float>(mouse_box_.x + (mouse_box_.w / 2.0f));
		float ty = static_cast<float>(mouse_box_.y + (mouse_box_.h / 2.0f));

		// direction = target - origin
		ray.dx_ = tx - ray.origin_.x;
		ray.dy_ = ty - ray.origin_.y;

		// inverse direction
		ray.inverse_.x = 1.0f / ray.dx_;
		ray.inverse_.y = 1.0f / ray.dy_;

		for (Cell& cell : board_)
		{
			if (!cell.is_wall_) continue;

			if (AABBTavianIntersection(cell.rect_, ray))
			{
				cell.highlighted_ = true;
			}
			else
			{
				cell.highlighted_ = false;
			}
		}
	}
}

bool Game::AABBTavianIntersection(SDL_Rect rect, Ray ray)
{
	const float tx1 = (rect.x - ray.origin_.x) * ray.inverse_.x;
	const float tx2 = ((rect.x + rect.w) - ray.origin_.x) * ray.inverse_.x;

	const float ty1 = (rect.y - ray.origin_.y) * ray.inverse_.y;
	const float ty2 = ((rect.y + rect.h) - ray.origin_.y) * ray.inverse_.y;

	const float tmin = std::max(std::min(tx1, tx2), std::min(ty1, ty2));
	const float tmax = std::min(std::max(tx1, tx2), std::max(ty1, ty2));

	return tmax >= tmin;
}

void Game::Render()
{
	SDL_RenderSetViewport(renderer_, NULL);
	SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderClear(renderer_);

	RenderGrid();
	RenderCells();

	SDL_SetRenderDrawColor(renderer_, 0xff, 0x00, 0x00, 0xff);
	SDL_RenderFillRect(renderer_, &player_.box_);

	SDL_SetRenderDrawColor(renderer_, 0x00, 0xff, 0x00, 0xff);
	SDL_RenderFillRect(renderer_, &mouse_box_);

	if (render_line_)
	{
		SDL_SetRenderDrawColor(renderer_, 0x00, 0xff, 0xff, 0xff);
		SDL_RenderDrawLine(renderer_, player_.box_.x + (player_.box_.w / 2), player_.box_.y + (player_.box_.h / 2), mouse_box_.x + (mouse_box_.w / 2), mouse_box_.y + (mouse_box_.h / 2));
	}

	SDL_RenderPresent(renderer_);
}

void Game::RenderGrid()
{
	SDL_SetRenderDrawColor(renderer_, 0x14, 0x14, 0x14, 0xff);

	for (int y = 1; y < cells_height_; ++y)
	{
		SDL_RenderDrawLine(renderer_, 0, y * cell_size_, constants::screen_width, y * cell_size_);
	}

	for (int x = 1; x < cells_width_; ++x)
	{
		SDL_RenderDrawLine(renderer_, x * cell_size_, 0, x * cell_size_, constants::screen_height);
	}
}

void Game::RenderCells()
{
	for (int i = 0; i < cells_width_ * cells_height_; ++i)
	{
		if (board_[i].is_wall_)
		{
			if (board_[i].highlighted_)
			{
				SDL_SetRenderDrawColor(renderer_, 0x00, 0xff, 0x00, 0xff);
			}
			else
			{
				SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0xff, 0xff);
			}
			
			SDL_RenderFillRect(renderer_, &board_[i].rect_);
		}

	}
}