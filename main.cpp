#include <cstddef>
#include <cstdio>

#include <cstring>
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
#define MAX_CONNECTIONS 12
#define PATH_ALIIVE_TIME 0.6
#define LINE_WIDTH 8
#define MUSIC_VOLUME .7

static vec2 last_hover{};
static vec2 current_hover{};
static float hover_timer{};

static int map[MAP_SZ*MAP_SZ]{};
static int filled_map[MAP_SZ*MAP_SZ]{};

struct Connection {
	daa<vec2> points;
	vec2 start;
	vec2 end;
	int id;
	Color color;
	bool done;
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

struct Level {
	Connection connections[MAX_CONNECTIONS];
	int nc;
};

static Level levels[]{
	{
		.connections = {
			create(v2(1, 9), v2(9, 7), 1, RED),
			create(v2(8, 4), v2(9, 8), 2, BLUE),
			create(v2(8, 3), v2(1, 3), 3, GREEN),
			create(v2(2, 3), v2(7, 2), 4, RED),
			create(v2(3, 6), v2(6, 4), 5, BLUE),
			create(v2(4, 6), v2(5, 5), 6, GREEN),
			create(v2(1, 4), v2(3, 8), 7, RED),
			create(v2(1, 8), v2(5, 4), 8, BLUE),
			create(v2(9, 9), v2(2, 6), 9, GREEN),
		},
		.nc = 9,
	},
	{
		.connections = {
			create(v2(0, 3), v2(8, 9), 1, RED),
			create(v2(0, 5), v2(4, 5), 2, BLUE),
			create(v2(1, 1), v2(8, 6), 3, GREEN),
			create(v2(1, 5), v2(3, 3), 4, RED),
			create(v2(1, 8), v2(7, 8), 5, BLUE),
			create(v2(2, 6), v2(7, 9), 6, GREEN),
			create(v2(2, 8), v2(4, 8), 7, RED),
			create(v2(4, 3), v2(5, 6), 8, BLUE),
			create(v2(6, 2), v2(8, 8), 9, GREEN),
			create(v2(7, 2), v2(7, 6), 10, RED),
		},
		.nc = 10,
	},
	{
		.connections = {
			create(v2(0, 9), v2(3, 2), 1, RED),
			create(v2(0, 5), v2(9, 6), 2, BLUE),
			create(v2(1, 1), v2(1, 5), 3, GREEN),
			create(v2(1, 7), v2(6, 8), 4, RED),
			create(v2(2, 1), v2(6, 5), 5, BLUE),
			create(v2(2, 2), v2(6, 3), 6, GREEN),
			create(v2(2, 8), v2(3, 7), 7, RED),
			create(v2(3, 6), v2(4, 7), 8, BLUE),
			create(v2(3, 5), v2(5, 7), 9, GREEN),
			create(v2(4, 9), v2(7, 6), 10, RED),
			create(v2(6, 6), v2(8, 8), 11, BLUE),
		},
		.nc = 11,
	},	
	{
		.connections = {
			create(v2(0, 9), v2(9, 2), 1, RED),
			create(v2(1, 4), v2(7, 9), 2, BLUE),
			create(v2(2, 2), v2(4, 9), 3, GREEN),
			create(v2(2, 3), v2(7, 7), 4, RED),
			create(v2(2, 4), v2(4, 4), 5, BLUE),
			create(v2(2, 8), v2(3, 4), 6, GREEN),
			create(v2(4, 8), v2(6, 7), 7, RED),
			create(v2(5, 4), v2(6, 6), 8, BLUE),
		},
		.nc = 8,
	},
	{
		.connections = {
			create(v2(0, 6), v2(5, 5), 1, RED),
			create(v2(0, 7), v2(5, 8), 2, BLUE),
			create(v2(0, 8), v2(3, 9), 3, GREEN),
			create(v2(1, 8), v2(7, 9), 4, RED),
			create(v2(2, 0), v2(6, 8), 5, BLUE),
			create(v2(2, 1), v2(6, 0), 6, GREEN),
			create(v2(2, 3), v2(3, 2), 7, RED),
			create(v2(3, 0), v2(8, 5), 8, BLUE),
			create(v2(4, 0), v2(8, 1), 9, GREEN),
			create(v2(7, 7), v2(9, 9), 10, RED),
			create(v2(8, 7), v2(9, 8), 11, BLUE),
		},
		.nc = 11,
	},		
	{
		.connections = {
			create(v2(0, 0), v2(1, 8), 1, RED),
			create(v2(1, 1), v2(7, 2), 2, BLUE),
			create(v2(1, 4), v2(4, 3), 3, GREEN),
			create(v2(1, 9), v2(2, 3), 4, RED),
			create(v2(2, 4), v2(5, 3), 5, BLUE),
			create(v2(3, 8), v2(7, 6), 6, GREEN),
			create(v2(3, 7), v2(8, 4), 7, RED),
			create(v2(4, 7), v2(9, 0), 8, BLUE),
			create(v2(9, 1), v2(9, 9), 9, GREEN),
		},
		.nc = 9,
	},		
	{
		.connections = {
			create(v2(0, 2), v2(5, 0), 1, RED),
			create(v2(0, 3), v2(3, 3), 2, BLUE),
			create(v2(1, 7), v2(2, 9), 3, GREEN),
			create(v2(1, 8), v2(7, 2), 4, RED),
			create(v2(3, 2), v2(3, 4), 5, BLUE),
			create(v2(3, 5), v2(5, 1), 6, GREEN),
			create(v2(3, 8), v2(7, 5), 7, RED),
			create(v2(3, 9), v2(4, 2), 8, BLUE),
			create(v2(6, 0), v2(9, 0), 9, GREEN),
			create(v2(6, 6), v2(8, 2), 10, RED),
			create(v2(7, 8), v2(8, 5), 11, BLUE),
		},
		.nc = 11,
	},
	{
		.connections = {
			create(v2(1, 2), v2(7, 7), 1, RED),
			create(v2(1, 3), v2(1, 6), 2, BLUE),
			create(v2(1, 7), v2(3, 8), 3, GREEN),
			create(v2(1, 8), v2(4, 7), 4, RED),
			create(v2(4, 6), v2(6, 3), 5, BLUE),
			create(v2(1, 9), v2(6, 9), 6, GREEN),
			create(v2(4, 4), v2(6, 7), 7, RED),
			create(v2(4, 5), v2(5, 3), 8, BLUE),
			create(v2(5, 5), v2(5, 9), 9, GREEN),
			create(v2(8, 1), v2(6, 8), 10, RED),
		},
		.nc = 10,
	},		
	{
		.connections = {
			create(v2(0, 0), v2(3, 3), 1, RED),
			create(v2(0, 1), v2(2, 3), 2, BLUE),
			create(v2(0, 2), v2(8, 8), 3, GREEN),
			create(v2(0, 4), v2(3, 0), 4, RED),
			create(v2(1, 6), v2(2, 8), 5, BLUE),
			create(v2(2, 4), v2(3, 6), 6, GREEN),
			create(v2(5, 2), v2(8, 2), 7, RED),
			create(v2(5, 3), v2(8, 1), 8, BLUE),
			create(v2(6, 1), v2(7, 5), 9, GREEN),
			create(v2(8, 3), v2(9, 6), 10, RED),
		},
		.nc = 10,
	},
	{
		.connections = {
			create(v2(0, 1), v2(2, 9), 1, RED),
			create(v2(0, 8), v2(7, 6), 2, BLUE),
			create(v2(0, 9), v2(8, 8), 3, GREEN),
			create(v2(1, 6), v2(7, 4), 4, RED),
			create(v2(2, 2), v2(5, 4), 5, BLUE),
			create(v2(2, 4), v2(8, 7), 6, GREEN),
			create(v2(3, 7), v2(5, 7), 7, RED),
			create(v2(8, 2), v2(9, 0), 8, BLUE),
			create(v2(9, 1), v2(9, 5), 9, GREEN),
		},
		.nc = 9,
	}
};

static int level_id = -1;
static Level current_level;

static Connection *current_connection;

static Camera2D cam{};
static RenderTexture2D game, post_process_1;

static bool muted{};

// :load
static Font font16, font32, font64;
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
static Texture2D is_muted, not_muted;

static Sound add_point;
static Sound complete_point;
static Sound fail;
static Music loop_back;

static vec2 prev_hover_cell{};
static vec2 hover_cell{};

static float volume{};

// :level_anim
static float anim_time{};
static bool start_anim{};
static vec2 quad_info{};
static bool change_level{};
static vec2 text_info{};

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

enum ParticleType {
	NONE,
	PATH,
};

struct Particle {
	ParticleType type;
	vec2 pos, vel, dir, size;
	vec2 off;
	Color color;
	float t;
	bool valid;
};

#define MAX_PARTICLES 1024
struct ParticleSpawner {
	Particle particles[MAX_PARTICLES];
};
static ParticleSpawner particle_spawner{};

void update_particles() {
	for(auto &p : particle_spawner.particles) {
		if (!p.valid) continue;
		switch(p.type) {
			case NONE: break;
			case PATH: {
					if (p.dir.y != 0) {
						p.pos.x += (p.vel.x * GetFrameTime());
						p.pos.y += (p.vel.y * p.dir.y * GetFrameTime());
					} else {
						p.pos.x += (p.vel.x * p.dir.x * GetFrameTime());
						p.pos.y += (p.vel.y * GetFrameTime());						
					}
					p.t += GetFrameTime();
					p.size.x = 8 * (p.t / PATH_ALIIVE_TIME);
					if (p.t >= PATH_ALIIVE_TIME) {
						p.valid = false;
					}
			} break;
		}
	}
}

void render_particle() {
	for(auto p : particle_spawner.particles) {
		if (!p.valid) continue;
		switch(p.type) {
			case NONE: break;
			case PATH: {
					float alpha = 1 * (p.t / PATH_ALIIVE_TIME);
					DrawCircleV(p.pos, p.size.x, ColorAlpha(p.color, alpha));
			} break;
		}
	}
}

void add_particle(Particle particle) {
	for (auto &p : particle_spawner.particles) {
		if (p.valid) continue;
		p.valid = true;
		p.color = particle.color;
		p.pos = particle.pos;
		p.vel = particle.vel;
		p.dir = particle.dir;
		p.t = particle.t;
		p.size = particle.size;
		p.type = particle.type;
		p.off = particle.off;
		break;
	}
}

void add_particles(Particle particle, int amnt) {
	for (int i = 0; i < amnt; i+=1) {
		add_particle(particle);
	}
}

void next_level() {
	memset(map, 0, sizeof(int) * (MAP_SZ * MAP_SZ));
	memset(filled_map, 0, sizeof(int) * (MAP_SZ * MAP_SZ));
	current_level = levels[++level_id];
	
	for(int i = 0; i < current_level.nc; i+=1) {
		auto c = current_level.connections[i];
		int start_index = c.start.y * MAP_SZ + c.start.x;
		int end_index = c.end.y * MAP_SZ + c.end.x;
		map[start_index] = c.id;
		map[end_index] = c.id;
	}
}

void init() {
	//:init
	
	cam.offset = (window_size / 2) - (v2of(MAP_SZ) * CELL_SZ) / 2; 
	cam.zoom = 1;
	cam.rotation = 0;
	cam.target = {};

	game = LoadRenderTexture(window_size.x, window_size.y);
	post_process_1 = LoadRenderTexture(window_size.x, window_size.y);

	// :load
	font16 = LoadFontEx("./res/arial.ttf", 16, 0, 96);
	font32 = LoadFontEx("./res/arial.ttf", 32, 0, 96);
	font64 = LoadFontEx("./res/arial.ttf", 64, 0, 96);

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
	is_muted = LoadTexture("./res/mute.png");
	not_muted = LoadTexture("./res/not_mute.png");

	add_point = LoadSound("./res/add_point.wav");
	fail = LoadSound("./res/no.wav");
	complete_point = LoadSound("./res/complete.wav");

	loop_back = LoadMusicStream("./res/back.ogg");
	PlayMusicStream(loop_back);

	start_anim = true;
	quad_info.y = window_size.x;
}

void add_path_particle(vec2 pos, vec2 dir) {
	vec2 render_pos = pos * CELL_SZ;
	render_pos = render_pos + CELL_SZ / 2.f;
	render_pos = render_pos + v2(GetRandomValue(-10, 10), GetRandomValue(-10, 10));
	
	for (int i = 0; i < 10; i += 1) {
		Particle particle = {
			.type = PATH,
			.pos = render_pos,
			.vel = v2(GetRandomValue(-100, 100), GetRandomValue(-100, 100)),
			.dir = dir,
			.size = v2of(8),
			.off = dir.x != 0 ? v2(GetRandomValue(-5, 5), 0) : v2(0, GetRandomValue(-5, 5)),
			.color = current_connection->color,
			.t = 0,
		};
		add_particle(particle);
	}
}

void update() {
	// :update

	if (muted) {
		volume = 0;
	} 
	volume = Lerp(volume, MUSIC_VOLUME, 0.1 * GetFrameTime());
	SetMusicVolume(loop_back, volume);
	UpdateMusicStream(loop_back);
	
	if (start_anim) {
		if (quad_info.y < window_size.x) {
			quad_info.y += 50;
		} else {
			if (!change_level) {
				change_level = true;
				next_level();
			}
			anim_time += GetFrameTime();
		}
		
		if (anim_time > 1) {
			
			quad_info.x += 50;
		}

		if (quad_info.x > window_size.x && anim_time > 2) {
			start_anim = false;
			change_level = false;
			quad_info = Vector2Zero();
			anim_time = 0;
		}
		
		return;
	}
	
	hover_cell = GetScreenToWorld2D(GetMousePosition(), cam);
	hover_cell.x = c(int, hover_cell.x) >> 5;
	hover_cell.y = c(int, hover_cell.y) >> 5;

	auto in_bounds = [](vec2 hover_cell){
		return hover_cell.x >= -1 && hover_cell.x <= 10 && hover_cell.y >= -1 && hover_cell.y <= 10;
	};

	auto id_at = [](vec2 at) {
		return map[int(at.y * MAP_SZ + at.x)];
	};
	
	auto is_free = [](vec2 at) {
		return filled_map[int(at.y * MAP_SZ + at.x)] == 0;
	};
	

	// Is connnection

	if (in_bounds(hover_cell)) {
		hover_cell.x = Clamp(hover_cell.x, 0, MAP_SZ - 1);
		hover_cell.y = Clamp(hover_cell.y, 0, MAP_SZ - 1);
	
		if (current_connection == NULL && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			for (auto &c : current_level.connections) {
				// Check if hover is either start or end
				if ((hover_cell == c.start || hover_cell == c.end) && id_at(hover_cell) == c.id) {
					current_connection = &c;
					if (c.points.count > 0) {
						for (int i = 0; i < current_connection->points.count; i+= 1) {
							vec2 p = current_connection->points.items[i];
							filled_map[int(p.y * MAP_SZ + p.x)] = 0;	
						}
						c.done = false;	
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
			if (is_free(hover_cell) && (id_at(hover_cell) == 0 || id_at(hover_cell) == current_connection->id)) {
				vec2 to_check = current_connection->points[0] == current_connection->start ? current_connection->end : current_connection->start;
				if (hover_cell != to_check) {
					vec2 last_point = current_connection->points[current_connection->points.count-2];
					if (last_point == hover_cell) {
						current_connection->points.pop();
					} else {
						// Adding new point
						vec2 dir = Vector2Normalize(hover_cell - current_connection->points.last());
						dir = Vector2Negate(dir);
						current_connection->points.append(hover_cell);
						add_path_particle(hover_cell, dir);
						PlaySound(add_point);
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
			current_connection->done = true;
			current_connection = NULL;
			PlaySound(complete_point);
		}
	}
	prev_hover_cell = hover_cell;

	update_particles();
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
				for (auto c : current_level.connections) {
					if (c.points.count > 1) {
						vec2 last_point = c.points.items[0];
						for(int i = 1; i < c.points.count; i++) {
							DrawLineEx((last_point * CELL_SZ) + CELL_SZ / 2.f, (c.points.items[i] * CELL_SZ) + CELL_SZ / 2.f, LINE_WIDTH, c.color);
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
					
					if (hover_cell == v2(x, y)) {
						hover_timer = fmaxf(hover_timer + GetFrameTime(), .2);
					} else {
						hover_timer = fmaxf(hover_timer - GetFrameTime(), 0);
					}



					if (hover_cell == v2(x, y) && !FloatEquals(hover_timer, 0)) {
						Vector2 origin = v2of(64) / 2 * (0.8 + hover_timer) / 2;
						DrawTexturePro(tex, {0, 0, 64, 64}, 
						               {v2e((pos - 9) + (origin / 2)), 64 * (.8f + hover_timer), 64 * (.8f + hover_timer)},
						               origin,
						               0,
						               WHITE);
					} else {
						DrawTextureEx(tex, pos-9, 0, .8f, WHITE);
					}
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
#if 0
				if (at) {
					DrawText(TextFormat("%d", at), hover_cell.x + 10, hover_cell.y + 10, 10, ORANGE);
				}
#endif
				DrawRectangleLinesEx({hover_cell.x, hover_cell.y, CELL_SZ, CELL_SZ}, 2.f, at == 0 ? RED : GREEN);
			}

			render_particle();
		}
		EndMode2D();	

		
	}
	EndTextureMode();

	// :post_1
	BeginTextureMode(post_process_1);
	{
		ClearBackground(BLANK);
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

#if 0
		if (current_connection) {
			for (int i = 0; i < current_connection->points.count; i+=1) {
				DrawTextEx(font16, TextFormat("[%d] %f %f", i, current_connection->points.items[i].x, current_connection->points.items[i].y), v2(10, i * 16), 16, 2, WHITE);
			}
		}
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

			// :level
			if (level_id != 9){
				auto dnext = v4zw(100, 32);
				b_of(screen, &dnext);
				center_x(screen, &dnext);
				pad_b(&dnext, 10);
				
				bool enabled = true;
				for (auto c : current_level.connections) {
					if (c.id == 0) continue;
					if (!c.done) {
						enabled = false;
						break;
					}
				}
				
				if (ui_btn(font32, "Next", dnext, enabled, fail)) {
					start_anim = true;
				}
			}

			{
				auto dpos = v4(10, 10, 32, 32);
				if (CheckCollisionPointRec(GetMousePosition(), to_rect(dpos))) {
					if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
						muted = !muted;
					}
				}

				DrawTextureV(muted ? not_muted : is_muted, xyv4(dpos), WHITE);		
			}

			if (start_anim) {
				DrawRectangleV(v2(quad_info.x, 0), v2(quad_info.y, window_size.y), BEIGE);
				if (change_level && anim_time < 1) {
					if (anim_time < 0.5) {
						text_info.y = Lerp(text_info.y, 1, 0.1);
					} else {
						text_info.y = Lerp(text_info.y, 0, 0.1);
					}
					
					const char* text = TextFormat("LEVEL %d", level_id + 1);
					auto dlabel = text_size(font64, text);
				
					center(screen, &dlabel);
					Color c = ColorAlpha(WHITE, text_info.y);
					label(font64, text, xyv4(dlabel), WHITE);
				}
			}	
		}
	}
	EndDrawing();

	last_hover = current_hover;
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
