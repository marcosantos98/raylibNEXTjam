#include <cstddef>
#include <cstdio>

#include <raylib.h>
#include <raymath.h>

#include "arena.hpp"
#include "da.hpp"
#include "ui.hpp"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

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
	0,0,0,0,0,0,0,0,0,3,
	0,0,0,0,0,0,0,0,0,0,
};
static Camera2D cam{};
static RenderTexture2D game;
static Font font32;

static vec2 hover_cell{};
static daa<vec2> path_points;
static daa<vec2> path_reversed;
static vec2 p_pos{};
static vec2 new_p_pos{};
static int path_idx = 0;
static bool go_path = false;

static int n_step{};
static int max_step{};

static void reverse_path_points(daa<vec2> *path_points) {
	size_t left = 0;
    size_t right = path_points->count - 1;
    while (left < right) {
        vec2 temp = path_points->items[left];
        path_points->items[left] = path_points->items[right];
        path_points->items[right] = temp;

        ++left;
        --right;
    }
}

Color color_from_id(int id) {
	switch (id) {
		case 0: 
			return BLACK;
		case 1:
			return WHITE;
		case 2:
			return YELLOW;
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

	font32 = LoadFontEx("./res/arial.ttf", 32, 0, 96);

	path_points = make<vec2>(&allocator);

	n_step = 0;
	max_step = 5;
}

void update() {
	// :update
	hover_cell = GetScreenToWorld2D(GetMousePosition(), cam);
	hover_cell.x = c(int, hover_cell.x) >> 5;
	hover_cell.y = c(int, hover_cell.y) >> 5;

	if (hover_cell.x >= 0 && hover_cell.x <= 9 && hover_cell.y >= 0 && hover_cell.y <= 9) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			vec2 last_point = path_points.count == 0 ? p_pos : path_points.items[path_points.count-1];
			if (hover_cell.x == last_point.x || hover_cell.y == last_point.y) {
				path_points.append(hover_cell);
				n_step += 1;
			}
		}
	}

	if (go_path) {
		if (!Vector2Equals(p_pos, new_p_pos)) {
			p_pos = Vector2MoveTowards(p_pos, new_p_pos, 100 * GetFrameTime());
		} else if(path_reversed.count > 0) {
			new_p_pos = path_reversed.pop() * CELL_SZ;
			path_idx += 1;
		}
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
					DrawRectangleLinesEx({v2e(pos), v2e(size)}, 1.f, WHITE);
				}
			}

			if (hover_cell.x >= 0 && hover_cell.x <= 9 && hover_cell.y >= 0 && hover_cell.y <= 9) {
				hover_cell = hover_cell * CELL_SZ;
				DrawRectangleLinesEx({hover_cell.x, hover_cell.y, CELL_SZ, CELL_SZ}, 2.f, RED);
			}
			DrawRectangleV(p_pos, v2of(CELL_SZ), WHITE);
			
			if (!go_path) {
				vec2 last_point = p_pos;
				for (int i = 0; i < path_points.count; i++) {
					DrawLineV(last_point + v2of(CELL_SZ / 2), path_points.items[i] * CELL_SZ + v2of(CELL_SZ / 2), PINK);
					last_point = path_points.items[i] * CELL_SZ;
				}
			} else {
				DrawLineV(p_pos + v2of(CELL_SZ / 2), new_p_pos + v2of(CELL_SZ / 2), PURPLE);
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

		for (int i = 0; i < path_points.count ;i++) {
			Color color = BLACK;
			if (i == path_idx) color = GREEN;
			DrawTextEx(font32, TextFormat("[%d] %.2f,%.2f", i, path_points.items[i].x, path_points.items[i].y), v2(10, i * 32), 32, 2, color);
		}
		
		auto text = TextFormat("%.2f, %.2f", p_pos.x, p_pos.y);
		vec2 size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10}, 32, 2, BLACK);
		text = TextFormat("%d/%d", path_idx, path_points.count);
		size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10}, 32, 2, BLACK);
		text = TextFormat("%.2f, %.2f", new_p_pos.x, new_p_pos.y);
		size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10}, 32, 2, BLACK);

		// :ui
		{
			auto screen = v4(0, 0, window_size.x, window_size.y);
			auto map = v4(
					(window_size.x - (MAP_SZ * CELL_SZ)) * .5f,
					(window_size.y - (MAP_SZ * CELL_SZ)) * .5f,
					(MAP_SZ * CELL_SZ),
					(MAP_SZ * CELL_SZ));

			//bool enabled = path_idx < path_points.count;
			if (ui_btn(font32, "Step", v4((window_size.x - 200) * .5f, window_size.y - 35, 200, 25))) {
				go_path = true;
				reverse_path_points(&path_points);
				path_reversed = path_points;
			}
			
			{
				auto text = TextFormat("Steps: %d/%d", n_step, max_step);
				auto dstep = text_size(font32, text);
				center_x(screen, &dstep);
				label(font32, text, xyv4(dstep), n_step == max_step ? RED : BLACK);
			}

			{
				auto dback = map;
				dback.y -= 34;
				dback.z = 32;
				dback.w = 32;
				if (ui_btn(font32, "<", dback)) {
					if (path_idx != 0) {
						path_idx -= 1;
						new_p_pos = path_reversed.items[path_idx] * CELL_SZ;
					}
				}
				
				dback.x += 42;
				if (ui_btn(font32, ">", dback)) {
				
				}
			}
		}
	}
	EndDrawing();
}

#if defined(PLATFORM_WEB)
void update_frame() {
	update();
	render();
}
#endif 

int main(void) {

	// :raylib
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(window_size.x, window_size.y, "Raylib Next Jam");
	InitAudioDevice();
	SetTargetFPS(60);
	SetExitKey(KEY_Q);

	init();

#if defined (PLATFORM_WEB)
	emscripten_set_main_loop(update_frame, 60, 1);	
#else
	while(!WindowShouldClose()) {
		update();
		render();
	}
#endif
	CloseWindow();
}
