#ifndef GAME_HPP
#define GAME_HPP

#include <SDL.h>

#include <vector>
#include <cmath>

struct Cell
{
	SDL_Rect rect_;
	bool is_wall_;
	bool highlighted_;
};

struct PlayerBox
{
	SDL_Rect box_;

	int vx_;
	int vy_;
};

struct Ray
{
	SDL_FPoint origin_;
	SDL_FPoint inverse_;
	float dx_;
	float dy_;
};

template <typename T>
class Vector2d
{
public:
	T x;
	T y;

	void Normalize()
	{
		T length = GetLength();
		x /= length;
		y /= length;
	}

	void SetLength(T length)
	{
		Normalize();
		x *= length;
		y *= length;
	}

	T GetLength()
	{
		return std::sqrt((x * x) + (y * y));
	}
};

class Game
{
private:
	bool initialized_;
	bool running_;
	int cell_size_;
	int cells_width_;
	int cells_height_;

	bool mouse_left_pressed_;
	bool mouse_right_pressed_;
	bool setting_walls_;
	bool render_line_;

	std::vector<Cell> board_;
	PlayerBox player_;
	SDL_Rect mouse_box_;
	SDL_Point mouse_position_;

	SDL_Window* window_;
	SDL_Renderer* renderer_;

public:
	Game();

	~Game();

	bool Initialize();

	void Finalize();

	void Run();

	void HandleEvents();
	
	void Tick();

	bool AABBTavianIntersection(SDL_Rect rect, Ray ray);
	
	void Render();

	void RenderGrid();
	
	void RenderCells();
};

#endif