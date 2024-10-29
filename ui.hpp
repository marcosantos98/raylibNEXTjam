#pragma once

#include <cmath>

#include <raylib.h>

typedef Vector2 vec2;

#define c(T, x) static_cast<T>(x)
#define v2(x, y) Vector2{(float)x, (float)y}
#define v2of(x) v2(x, x)
#define v2e(v) (v).x, (v).y

static vec2 window_size = v2(1152, 648);

static vec2 operator/(vec2 self, float other) {
	return {self.x / other, self.y / other};
}

static vec2 operator*(vec2 self, float other) {
	return {self.x * other, self.y * other};
}

static vec2 operator-(vec2 self, vec2 other) {
	return {self.x - other.x, self.y - other.y};
}

static vec2 operator+(vec2 self, vec2 other) {
	return {self.x + other.x, self.y + other.y};
}

static bool operator==(vec2 self, vec2 other) {
	return self.x == other.x && self.y == other.y;
}

static bool operator !=(vec2 self, vec2 other) {
	return !(self == other);
}

typedef Vector4 vec4;

#define to_rect(vf) Rectangle{vf.x, vf.y, vf.z, vf.w}
#define v4(x, y, z, w) vec4{(float)x, (float)y, (float)z, (float)w}
#define xyv4(vf) v2(vf.x, vf.y)
#define zwv4(vf) v2(vf.z, vf.w)
#define v4zw(z, w) v4(0, 0, z, w)

template <typename A, typename B>
struct pair {
	A a;
	B b;
};

inline pair<bool, bool> check_hover_click(vec4 dest) {
	bool hover = false;
	bool click = false;
	if (CheckCollisionPointRec(GetMousePosition(), to_rect(dest))) {
		hover = true;
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			click = true;
		}
	}
	return {
		.a = hover,
		.b = click
	};
}

static bool ui_btn(Font font, const char* text, vec4 dest, bool enabled = true) {
	auto [hover, click] = check_hover_click(dest);

	vec2 text_size = MeasureTextEx(font, text, font.baseSize, 2.f);	
	vec2 text_pos = xyv4(dest) + v2(((dest.z - text_size.x) * .5f), ((dest.w - text_size.y) * .5f)); 

	DrawRectangleRec(to_rect(dest), hover ? ColorBrightness(RED, .3) : RED);
	DrawTextEx(font, text, text_pos, 32, 2, WHITE);

	return click && enabled;
}

static vec4 text_size(Font font, const char* text) {
	vec2 a = MeasureTextEx(font, text, font.baseSize, 2);
	return v4zw(a.x, a.y);
}

static void label(Font font, const char* text, vec2 pos, Color color = WHITE) {

	DrawTextEx(font, text, pos, font.baseSize, 2, color);
}

static void center_x(vec4 where, vec4* who) {
	who->x += floorf((where.z - who->z) * .5);
}
