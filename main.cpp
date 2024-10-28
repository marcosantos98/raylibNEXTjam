#include <cstdio>

#include <raylib.h>
#include <raymath.h>

#include "arena.hpp"
#include "da.hpp"

#define c(T, x) static_cast<T>(x)
#define v2(x, y) Vector2{(float)x, (float)y}
#define v2of(x) v2(x, x)

typedef Vector2 vec2;

vec2 window_size = v2(1152, 648);

vec2 operator/(vec2 self, float other) {
	return {self.x / other, self.y / other};
}

vec2 operator*(vec2 self, float other) {
	return {self.x * other, self.y * other};
}

vec2 operator-(vec2 self, vec2 other) {
	return {self.x - other.x, self.y - other.y};
}

vec2 operator+(vec2 self, vec2 other) {
	return {self.x + other.x, self.y + other.y};
}

GrowingArena allocator;
GrowingArena temp_allocator;

template<typename T>
T alloc(size_t size, GrowingArena* arena = &allocator) {
	return arena_alloc<T>(arena, size);
}

// :data
#define MAP_SZ 10
#define CELL_SZ 32

static int map[MAP_SZ*MAP_SZ]{
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,2,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
};
static Camera2D cam{};
static RenderTexture2D game;

static vec2 hover_cell{};
static daa<vec2> path_points;
static vec2 p_pos{};
static int path_idx{};

Color color_from_id(int id) {
	switch (id) {
		case 0: 
			return BLACK;
		case 1:
			return WHITE;
		case 2:
			return RED;
	}
	return PINK;
}

void init() {
	//:init
	
	cam.offset = (window_size / 2) - (v2of(MAP_SZ) * CELL_SZ) / 2; 
	cam.zoom = 1;
	cam.rotation = 0;
	cam.target = {};

	game = LoadRenderTexture(window_size.x, window_size.y);

	path_points = make<vec2>(&allocator);
	path_points.append(v2of(0));
}

void update() {
	// :update
	hover_cell = GetScreenToWorld2D(GetMousePosition(), cam);
	hover_cell.x = c(int, hover_cell.x) >> 5;
	hover_cell.y = c(int, hover_cell.y) >> 5;

	if (hover_cell.x >= 0 && hover_cell.x <= 9 && hover_cell.y >= 0 && hover_cell.y <= 9) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			vec2 last_point = path_points.items[path_points.count-1];
			if (hover_cell.x == last_point.x || hover_cell.y == last_point.y)
				path_points.append(hover_cell);
		}
	}

	if (IsKeyPressed(KEY_S)) {
		p_pos = Vector2MoveTowards(p_pos, path_points.items[path_idx++], 100 * GetFrameTime());
	}
}

void render() {
	// :render
	
	BeginTextureMode(game);
	{
		ClearBackground(BLANK);
		BeginMode2D(cam);
		{
			// :map
			for (int y = 0; y < MAP_SZ; y++) {
				for (int x = 0; x < MAP_SZ; x++) {
					int at = map[y*MAP_SZ+x];
					Color color = color_from_id(at);
					vec2 pos = v2(x, y) * CELL_SZ;
					vec2 size = v2of(CELL_SZ);
					DrawRectangleV(pos, size, color);
				}
			}

			hover_cell = hover_cell * CELL_SZ;
			DrawRectangleLinesEx({hover_cell.x, hover_cell.y, CELL_SZ, CELL_SZ}, 2.f, RED);

			DrawRectangleV(p_pos * CELL_SZ, v2of(CELL_SZ), WHITE);

			for (int i = 1; i < path_points.count; i++) {
				DrawLineV(path_points.items[i-1] * CELL_SZ + v2of(CELL_SZ / 2), path_points.items[i] * CELL_SZ + v2of(CELL_SZ / 2), PINK);
			}
		}
		EndMode2D();	
	}
	EndTextureMode();

	BeginDrawing();
	{
		ClearBackground(GRAY);
		DrawTexturePro(game.texture, 
				{0, 0, c(float, game.texture.width), c(float, -game.texture.height)},
				{0, 0, c(float, game.texture.width), c(float, game.texture.height)},
				{0, 0},
				0,
				WHITE);
	}
	EndDrawing();
}

int main(void) {

	// :raylib
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(window_size.x, window_size.y, "Raylib Next Jam");
	InitAudioDevice();
	SetTargetFPS(60);
	SetExitKey(KEY_Q);

	init();

	while(!WindowShouldClose()) {
		update();
		render();
	}

	CloseWindow();
}
