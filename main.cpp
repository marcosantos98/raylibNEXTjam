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
T* alloc(size_t size, GrowingArena* arena = &allocator) {
	return arena_alloc<T>(arena, size);
}

// :data
#define MAP_SZ 10
#define CELL_SZ 32
#define V2_ZERO v2of(0)
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

static Rectangle activator{};

static Camera2D cam{};
static RenderTexture2D game;
static Font font16, font32;
static Texture2D back, flower;

static vec2 hover_cell{};
static daa<vec2> path_points;
static daa<vec2> path_reversed;
static vec2 p_pos{};
static vec2 new_p_pos{};
static int path_idx = 0;
static int path_cursor{};
static bool go_path = false;

static int n_step{};
static int max_step{};

struct PathPoint {
	vec2 val;
	PathPoint* next;
	PathPoint* prev;
};

struct Path {
	PathPoint *current;
};

void add_path(Path* path, vec2 what) {
	PathPoint *p = alloc<PathPoint>(sizeof(PathPoint));
	p->val = what;

	if (path->current) {
		path->current->next = p;
		p->prev = path->current;
		path->current = p;
	}	else {
		path->current = p;
	}
}

Path create_path(vec2 start) {
	Path p{};

	add_path(&p, start);

	return p;
}

static Path path{};

vec2 path_back() {	
	path.current = path.current->prev;
	return path.current->val;
}

vec2 path_front() {
	path.current = path.current->next;
	return path.current->val;
}

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

void printv(vec2 v) {
	printf("%f, %f\n", v.x, v.y);
}

struct Activator {
	vec2 pos;
	bool activated;
};

static Activator activators[]{
	{.pos = v2of(5) * CELL_SZ, .activated = false},
};

static vec2 end_point = v2(9, 8) * CELL_SZ;
static bool at_end = false;
static bool completed = false;

void init() {
	//:init
	
	cam.offset = (window_size / 2) - (v2of(MAP_SZ) * CELL_SZ) / 2; 
	cam.zoom = 1;
	cam.rotation = 0;
	cam.target = {};

	game = LoadRenderTexture(window_size.x, window_size.y);

	font16 = LoadFontEx("./res/arial.ttf", 16, 0, 96);
	font32 = LoadFontEx("./res/arial.ttf", 32, 0, 96);

	back = LoadTexture("./res/thing.png");
	flower = LoadTexture("./res/flower.png");

	path = create_path(p_pos);

	path_points = make<vec2>(&allocator);
	
	path_points.append(p_pos);
	
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
			if (hover_cell!=last_point && (hover_cell.x == last_point.x || hover_cell.y == last_point.y)) {
				path_points.append(hover_cell);
				n_step += 1;
			}
		}
	}

	// :path
	{
		if (go_path) {
			if (!Vector2Equals(p_pos, new_p_pos)) {
				p_pos = Vector2MoveTowards(p_pos, new_p_pos, 500 * GetFrameTime());

				// :activator
				Rectangle p_rect{v2e(p_pos), v2e(v2of(CELL_SZ))};
				for (auto &activator : activators) {
					Rectangle activator_rect{v2e(activator.pos), v2e(v2of(CELL_SZ))};
					if (!activator.activated && CheckCollisionRecs(activator_rect, p_rect)) {
						printf("Activated!\n");
						activator.activated = true;
					}
				}

				if (Vector2Equals(p_pos, end_point)) {
					at_end = true;
				}
				
			} else if(path_reversed.count > 0) {
				new_p_pos = path_reversed.pop() * CELL_SZ;
				path_idx += 1;
				add_path(&path, new_p_pos);
			}
		}
	}

	if (at_end) {
		bool all_activated = true;
		for (auto activator : activators) {
			if (!activator.activated) {
				all_activated = false;
				break;
			}
		}
		if (all_activated) {
			completed = true;
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

			DrawTexture(back, -180/2, -180/2, WHITE);
			
			// :map
			for (int y = 0; y < MAP_SZ; y++) {
				for (int x = 0; x < MAP_SZ; x++) {
					int at = map[y*MAP_SZ+x];
					vec2 pos = v2(x, y) * CELL_SZ;
					if (at == 0) continue;
					if (at != 2) {
						Color color = color_from_id(at);
						vec2 size = v2of(CELL_SZ);
						DrawRectangleV(pos, size, color);
						//DrawRectangleLinesEx({v2e(pos), v2e(size)}, 1.f, WHITE);
					} else {
						DrawTextureV(flower, pos, WHITE);
					}
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
		ClearBackground(BLACK);
		DrawTexturePro(game.texture, 
				{0, 0, c(float, game.texture.width), c(float, -game.texture.height)},
				{0, 0, c(float, game.texture.width), c(float, game.texture.height)},
				{0, 0},
				0,
				WHITE);

#if 0
		for (int i = 0; i < path_points.count ;i++) {
			Color color = WHITE;
			if (i == path_idx) color = GREEN;
			DrawTextEx(font32, TextFormat("[%d] %.2f,%.2f", i, path_points.items[i].x, path_points.items[i].y), v2(10, i * 32), 32, 2, color);
		}

		auto text = TextFormat("%.2f, %.2f", p_pos.x, p_pos.y);
		vec2 size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10}, 32, 2, WHITE);
		text = TextFormat("%d/%d", path_idx, path_points.count);
		size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10}, 32, 2, WHITE);
		text = TextFormat("%.2f, %.2f", new_p_pos.x, new_p_pos.y);
		size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10}, 32, 2, WHITE);
		text = TextFormat("Activator: %d", activators[0].activated);
		size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10 + 32 + 10}, 32, 2, WHITE);
		text = TextFormat("Reached end: %d", at_end);
		size = MeasureTextEx(font32, text, 32, 2);
		DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10 + 32 + 10 + 32 + 10}, 32, 2, WHITE);
		// :debug
#endif

		// :ui
		{
			auto screen = v4(0, 0, window_size.x, window_size.y);
			auto map = v4(
					(window_size.x - (MAP_SZ * CELL_SZ)) * .5f,
					(window_size.y - (MAP_SZ * CELL_SZ)) * .5f,
					(MAP_SZ * CELL_SZ),
					(MAP_SZ * CELL_SZ));

			bool enabled = path_points.count > 1;
			if (ui_btn(font32, "Step", v4((window_size.x - 200) * .5f, window_size.y - 35, 200, 25), enabled)) {
				go_path = true;
				reverse_path_points(&path_points);
				path_reversed = path_points;
			}
			
			{
				auto text = TextFormat("Steps: %d/%d", n_step, max_step);
				auto dstep = text_size(font32, text);
				center_x(screen, &dstep);
				label(font32, text, xyv4(dstep), n_step == max_step ? RED : WHITE);
			}

			// :undo
			{
				auto dback = map;
				dback.y -= 34;
				dback.z = 32;
				dback.w = 32;
				if (ui_btn(font32, "<", dback)) {
					new_p_pos = path_back();
				}
				
				dback.x += 42;
				if (ui_btn(font32, ">", dback)) {
					new_p_pos = path_front();
				}

				dback.x += 42;
				if (ui_btn(font32, "R", dback)) {
					path_points.clear();
					p_pos = V2_ZERO;
					path = create_path(V2_ZERO);
					n_step = 0;
					go_path = false;
					path_idx = 0;
				}
			}

			// :objective
			{
				const char* text = "Reach the pink thing with the available steps.";
				auto goal_size = zwv4(text_size(font16, text));
				vec2 dgoal{10, window_size.y - 10 - goal_size.y};
				label(font16, text, dgoal, BLACK);
			}

			// :level
			{
				auto dnext = v4zw(100, 32);
				br_of(screen, &dnext);
				pad_br(&dnext, 10);

				if (ui_btn(font32, "Next", dnext, completed)) {
					
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
