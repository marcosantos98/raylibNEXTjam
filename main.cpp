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
#define INV v2of(-1)

static int map[MAP_SZ*MAP_SZ]{
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,4,0,0,
	0,3,4,0,0,0,0,0,3,0,
	0,7,0,0,0,8,5,0,2,0,
	0,0,0,0,0,6,0,0,0,0,
	0,0,9,5,6,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,1,
	0,8,0,7,0,0,0,0,0,2,
	0,1,0,0,0,0,0,0,0,9,
};

static int filled_map[MAP_SZ*MAP_SZ]{};

struct Connection {
	daa<vec2> points;
	vec2 start;
	vec2 end;
	int id;
	Color color;
};

Connection create(vec2 start, vec2 end, int id, Color color) {
	Connection c{};
	c.points = make<vec2>(&allocator);
	c.start = start;
	c.end = end;
	c.id = id;
	c.color = color;	
	return c;
}

static Connection connections[]{
	create(v2(1, 9), v2(9, 7), 1, PINK),
	create(v2(8, 4), v2(9, 8), 2, BLUE),
	create(v2(8, 3), v2(1, 3), 3, GREEN),
	create(v2(2, 3), v2(7, 2), 4, PINK),
	create(v2(3, 6), v2(6, 4), 5, BLUE),
	create(v2(4, 6), v2(7, 5), 6, GREEN),
	create(v2(1, 4), v2(3, 8), 7, PINK),
	create(v2(1, 8), v2(5, 4), 8, BLUE),
	create(v2(9, 9), v2(2, 6), 9, GREEN),
};

static Connection *current_connection;

static Camera2D cam{};
static RenderTexture2D game, post_process_1;

// :load
static Font font16, font32;
static Texture2D back, spot_back;
static Texture2D flower_0;
static Texture2D flower_1;
static Texture2D flower_2;
static Texture2D flower_3;
static Texture2D flower_4;
static Texture2D flower_5;
static Texture2D flower_6;
static Texture2D flower_7;
static Texture2D flower_8;
static Texture2D flower_9;
static Texture2D flower_10;
static Texture2D flower_11;

static vec2 prev_hover_cell{};
static vec2 hover_cell{};

static daa<vec2> current_points;
static bool start_line{};
static int current_i{};
static vec2 end_line{};
static bool completed = false;

Texture tex_from_id(int id) {
	switch (id) {
		case 1:
			return flower_0;
		case 2:
			return flower_1;
		case 3:
			return flower_2;
		case 4:
			return flower_3;
		case 5:
			return flower_4;
		case 6:
			return flower_5;
		case 7:
			return flower_6;
		case 8:
			return flower_7;
		case 9:
			return flower_8;
		case 10:
			return flower_9;
		case 11:
			return flower_10;
		case 12:
			return flower_11;
	}
	return {};
}

void printv(vec2 v) {
	printf("%f, %f\n", v.x, v.y);
}

void init() {
	//:init
	
	cam.offset = (window_size / 2) - (v2of(MAP_SZ) * CELL_SZ) / 2; 
	cam.zoom = 1;
	cam.rotation = 0;
	cam.target = {};

	game = LoadRenderTexture(window_size.x, window_size.y);
	post_process_1 = LoadRenderTexture(window_size.x, window_size.y);

	
	end_line = INV;

	current_points = make<vec2>(&allocator);
	
	// :load
	font16 = LoadFontEx("./res/arial.ttf", 16, 0, 96);
	font32 = LoadFontEx("./res/arial.ttf", 32, 0, 96);

	back = LoadTexture("./res/thing.png");
	flower_0 = LoadTexture("./res/flower_0.png");
	flower_1 = LoadTexture("./res/flower_1.png");
	flower_2 = LoadTexture("./res/flower_2.png");
	flower_3 = LoadTexture("./res/flower_3.png");
	flower_4 = LoadTexture("./res/flower_4.png");
	flower_5 = LoadTexture("./res/flower_5.png");
	flower_6 = LoadTexture("./res/flower_6.png");
	flower_7 = LoadTexture("./res/flower_7.png");
	flower_8 = LoadTexture("./res/flower_8.png");
	flower_9 = LoadTexture("./res/flower_9.png");
	flower_10 = LoadTexture("./res/flower_10.png");
	flower_11 = LoadTexture("./res/flower_11.png");
	spot_back = LoadTexture("./res/spot_back_w.png");
}

void update() {
	// :update
	hover_cell = GetScreenToWorld2D(GetMousePosition(), cam);
	hover_cell.x = c(int, hover_cell.x) >> 5;
	hover_cell.y = c(int, hover_cell.y) >> 5;

	auto in_bounds = [](vec2 hover_cell){
		return hover_cell.x >= 0 && hover_cell.x <= 9 && hover_cell.y >= 0 && hover_cell.y <= 9;
	};

	auto id_at = [](vec2 at) {
		return map[int(at.y * MAP_SZ + at.x)];
	};
	
	auto is_free = [](vec2 at) {
		return filled_map[int(at.y * MAP_SZ + at.x)] == 0;
	};
	
	if (in_bounds(hover_cell)) {

		// Is connnection
		if (current_connection == NULL && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			for (auto &c : connections) {
				// Check if hover is either start or end
				if ((hover_cell == c.start || hover_cell == c.end) && id_at(hover_cell) == c.id) {
					current_connection = &c;
					if (c.points.count > 0) {
						for (int i = 0; i < current_connection->points.count; i+= 1) {
							vec2 p = current_connection->points.items[i];
							filled_map[int(p.y * MAP_SZ + p.x)] = 0;	
						}	
						c.points.clear();				
					} else {
						c.points.append(hover_cell);
					}
					break;				
				}
			}
		}

		
		// Have connection append to it!
		bool has_target{};
		if (current_connection && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && prev_hover_cell != hover_cell) {
			if (is_free(hover_cell)) {
				vec2 to_check = current_connection->points[0] == current_connection->start ? current_connection->end : current_connection->start;
				if (hover_cell != to_check) {
					vec2 last_point = current_connection->points[current_connection->points.count-2];
					if (last_point == hover_cell) {
						current_connection->points.pop();
					} else {
						current_connection->points.append(hover_cell);
					}
				} else if(hover_cell == to_check && id_at(hover_cell) == current_connection->id) {
					current_connection->points.append(hover_cell);
					has_target = true;
				}
			} else {
				for (int i = 0; i < current_connection->points.count; i+= 1) {
					vec2 p = current_connection->points.items[i];
					filled_map[int(p.y * MAP_SZ + p.x)] = 0;	
				}
				current_connection->points.clear();
				current_connection = NULL;
			}
		}

		// Check if is one of start points or remove!
		if (current_connection && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			if (id_at(hover_cell) != current_connection->id) {
				current_connection->points.clear();
				current_connection = NULL;
			} else {
				has_target = true;
			}
		}

		if (has_target) {
			for (int i = 0; i < current_connection->points.count; i+= 1) {
				vec2 p = current_connection->points.items[i];
				filled_map[int(p.y * MAP_SZ + p.x)] = 1;	
			}
			current_connection = NULL;
		}
	} 
	

	prev_hover_cell = hover_cell;
}

void render() {
	// :render
	
	BeginTextureMode(game);
	{
		ClearBackground(BLANK);
		BeginMode2D(cam);
		{

			DrawRectangle(-90, -90, 500, 500, BROWN);

			// :spots
			for (int y = 0; y < MAP_SZ; y++) {
				for (int x = 0; x < MAP_SZ; x++) {
					vec2 pos = v2(x, y) * CELL_SZ;
					DrawTextureV(spot_back, pos, WHITE);
				}
			}	
			// :line
			{
				for (auto c : connections) {
					if (c.points.count > 1) {
						vec2 last_point = c.points.items[0];
						for(int i = 1; i < c.points.count; i++) {
							DrawLineEx((last_point * CELL_SZ) + CELL_SZ / 2.f, (c.points.items[i] * CELL_SZ) + CELL_SZ / 2.f, 6.f, c.color);
							last_point = c.points.items[i];
						}
					}
				}
			}
			
			// :map
			for (int y = 0; y < MAP_SZ; y++) {
				for (int x = 0; x < MAP_SZ; x++) {
					int at = map[y*MAP_SZ+x];
					vec2 pos = v2(x, y) * CELL_SZ;
					if (at == 0) continue;
					Texture tex = tex_from_id(at);
					vec2 size = v2of(CELL_SZ);
					DrawTextureEx(tex, pos-9, 0, .8f, WHITE);
				}
			}
#if 0
			for (int y = 0; y < MAP_SZ; y++) {
				for (int x = 0; x < MAP_SZ; x++) {
					int at = filled_map[y*MAP_SZ+x];
					int at2 = map[y*MAP_SZ+x];
					vec2 pos = v2(x, y) * CELL_SZ;
					if (at == 0) continue;
					vec2 size = v2of(CELL_SZ);
					DrawRectangleV(pos, size,  WHITE);
					DrawText(TextFormat("%d", at2), pos.x, pos.y, 10, BLACK);
				}
			}
#endif	
			DrawTexture(back, -180/2, -180/2, WHITE);
			
			if (hover_cell.x >= 0 && hover_cell.x <= 9 && hover_cell.y >= 0 && hover_cell.y <= 9) {
				int at = map[int(hover_cell.y * MAP_SZ + hover_cell.x)];
				hover_cell = hover_cell * CELL_SZ;
				DrawRectangleLinesEx({hover_cell.x, hover_cell.y, CELL_SZ, CELL_SZ}, 2.f, at == 0 ? RED : GREEN);
			}
		}
		EndMode2D();	
	}
	EndTextureMode();

	// :post_1
	BeginTextureMode(post_process_1);
	{
		DrawTexturePro(game.texture, 
			{0, 0, c(float, game.texture.width), c(float, -game.texture.height)},
			{0, 0, c(float, game.texture.width), c(float, game.texture.height)},
			{0, 0},
			0,
			WHITE);
	}
	EndTextureMode();

	RenderTexture final = post_process_1;

	BeginDrawing();
	{
		ClearBackground(BLACK);
		DrawTexturePro(final.texture, 
				{0, 0, c(float, final.texture.width), c(float, -final.texture.height)},
				{0, 0, c(float, final.texture.width), c(float, final.texture.height)},
				{0, 0},
				0,
				WHITE);

#if 1
		if (current_connection) {
			for (int i = 0; i < current_connection->points.count; i+=1) {
				DrawTextEx(font16, TextFormat("[%d] %f %f", i, current_connection->points.items[i].x, current_connection->points.items[i].y), v2(10, i * 16), 16, 2, WHITE);
			}
		}
		// auto text = TextFormat("%.2f, %.2f", start_line.x, start_line.y);
		// vec2 size = MeasureTextEx(font32, text, 32, 2);
		// DrawTextEx(font32, text, {window_size.x - size.x - 10, 10}, 32, 2, WHITE);
		// text = TextFormat("%.2f/%.2f", end_line.x, end_line.y);
		// size = MeasureTextEx(font32, text, 32, 2);
		// DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10}, 32, 2, WHITE);
		// text = TextFormat("%.2f, %.2f", new_p_pos.x, new_p_pos.y);
		// size = MeasureTextEx(font32, text, 32, 2);
		// DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10}, 32, 2, WHITE);
		// text = TextFormat("Activator: %d", activators[0].activated);
		// size = MeasureTextEx(font32, text, 32, 2);
		// DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10 + 32 + 10}, 32, 2, WHITE);
		// text = TextFormat("Reached end: %d", at_end);
		// size = MeasureTextEx(font32, text, 32, 2);
		// DrawTextEx(font32, text, {window_size.x - size.x - 10, 10 + 32 + 10 + 32 + 10 + 32 + 10 + 32 + 10}, 32, 2, WHITE);
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

			// :fill
			{
				int filled{};
				for (int i = 0; i < MAP_SZ * MAP_SZ; i+=1) {
					if(filled_map[i] == 1) filled += 1;
				}
				float fill_percentage = (float(filled) / (MAP_SZ * MAP_SZ)) * 100.f;
				auto text = TextFormat("Fill: %.1f%%", fill_percentage);
				auto dfill = text_size(font32, text);
				center_x(screen, &dfill);

				label(font32, text, xyv4(dfill));
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
