#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include "Settings.hpp"



wchar_t* s2wc(const char* c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

namespace menu
{
	uobject* font;
	uintptr_t active_textbox = 0;

	flinearcolor RGBtoFLC(float r, float g, float b, float a = 1.0f)
	{
		float gamma = 1.8f;

		return {
			powf(r / 255.0f, gamma),
			powf(g / 255.0f, gamma),
			powf(b / 255.0f, gamma),
			a
		};
	}

	flinearcolor RGBtoFLC2(float r, float g, float b, float a = 1.0f)
	{
		return { r / 255, g / 255, b / 255, a };
	}

	void cooltext(ucanvas* canvas, const wchar_t* text, fvector2d pos) {
		canvas->k2_drawtext(menu::font, text, fvector2d(pos.x, pos.y), fvector2d(0.99, 0.96),
			flinearcolor{ 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, RGBtoFLC(0, 0, 0),
			fvector2d(0, 0), false, true, false, RGBtoFLC(0, 0, 0));
	}

	void draw_textyyy(ucanvas* canvas, uobject* font, const wchar_t* text, flinearcolor color, fvector2d pos, bool centered = false) {
		if (!canvas || !font || !text) return;

		canvas->k2_drawtext(
			font,
			text,
			pos,
			{ 1.00f, 1.00f },
			color,
			0.f,
			{ 0, 0, 0, 0.30f },
			{ 0, 0 },
			true,
			true,
			true,
			{ 0, 0, 0, 0.45f }
		);
	}

	fvector2d CalcTextSize(const wchar_t* text) {
		HDC hdc = GetDC(nullptr); 
		if (!hdc)
			return { 0, 0 };

		SIZE size;
		GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &size);
		ReleaseDC(nullptr, hdc);

		return fvector2d{ static_cast<float>(size.cx), static_cast<float>(size.cy) };
	}

	namespace input
	{
		bool mouseDown[5];
		bool mouseDownPrev[5];
		bool clickConsumed;

		bool is_any_mouse_down()
		{
			for (int i = 0; i < 5; i++) if (mouseDown[i]) return true;
			return false;
		}

		bool is_mouse_clicked(int button, int element_id, bool repeat)
		{
			// Check if button is pressed now and was NOT pressed last frame
			if (mouseDown[button] && !mouseDownPrev[button] && !clickConsumed)
			{
				clickConsumed = true;
				return true;
			}
			
			// Support for repeat if needed, but usually not for buttons/checkboxes
			if (repeat && mouseDown[button])
				return true;

			return false;
		}

		void handle()
		{
			for (int i = 0; i < 5; i++) mouseDownPrev[i] = mouseDown[i];

			mouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			mouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
			mouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
			mouseDown[3] = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
			mouseDown[4] = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
		}

		void ResetFrame()
		{
			clickConsumed = false;
		}
	}

	flinearcolor HSVtoRGB(float h, float s, float v) {
		float r, g, b;

		int i = static_cast<int>(h * 6);
		float f = h * 6 - i;
		float p = v * (1 - s);
		float q = v * (1 - f * s);
		float t = v * (1 - (1 - f) * s);

		switch (i % 6) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5: r = v; g = p; b = q; break;
		default: r = g = b = 0.0f;
		}

		return flinearcolor{ r, g, b, 1.0f };

	}

	// x9 ware
	/*namespace Colors
	{
		flinearcolor Text{ 192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 255.0f / 255.0f }; //
		flinearcolor Text_Shadow{ 0.0f, 0.0f, 0.0f, 1.0f };
		flinearcolor Text_Outline{ 0.0f, 0.0f, 0.0f, 1.0f };
		flinearcolor Text_Active = RGBtoFLC(38, 149, 255);  // Updated purple

		flinearcolor Button_Idle{ 10.0 / 255.0f, 10.0 / 255.0f, 10.0 / 255.0f, 0.8f }; //
		flinearcolor Button_Hovered = RGBtoFLC(7, 7, 7);
		flinearcolor Button_Active = RGBtoFLC(38, 149, 255);  // Updated purple

		flinearcolor Checkbox_Idle = RGBtoFLC(15, 15, 15);
		flinearcolor Checkbox_Hovered = RGBtoFLC(20, 20, 20);
		flinearcolor Checkbox_Enabled = RGBtoFLC(38, 149, 255); // Updated purple

		flinearcolor Combobox_Idle{ 200.0f / 255.0f, 150.0f / 255.0f, 40.0f / 255.0f };
		flinearcolor Combobox_Hovered{ 200.0f / 255.0f, 150.0f / 255.0f, 40.0f / 255.0f };
		flinearcolor Combobox_Elements{ 0.239f, 0.42f, 0.0f, 0.5f };

		flinearcolor Slider_Idle = RGBtoFLC(8, 8, 8);
		flinearcolor Slider_Hovered = RGBtoFLC(14, 14, 14);
		flinearcolor Slider_Progress = RGBtoFLC(38, 149, 255);  // Updated purple
		flinearcolor Slider_Button = RGBtoFLC(38, 149, 255);  // Updated purple

		flinearcolor ColorPicker_Background{ 0.006f, 0.006f, 0.006f, 0.4f };
	}*/

	namespace Accent
	{
		// Active UI accent = white (checkboxes, sliders, selected items)
		flinearcolor Red = RGBtoFLC2(255, 255, 255);
		flinearcolor Bright = RGBtoFLC2(255, 255, 255);
		flinearcolor Dim = RGBtoFLC2(180, 180, 180);
		flinearcolor Background = RGBtoFLC(22, 22, 26);
		flinearcolor Panel = RGBtoFLC(28, 28, 32);
		flinearcolor Border = RGBtoFLC(210, 210, 215);
		flinearcolor Text_Dim = RGBtoFLC(140, 140, 150);
		flinearcolor Text_Main = RGBtoFLC(220, 220, 228);
	}

	namespace Colors
	{
		flinearcolor Text{ 1.0f, 1.0f, 1.0f, 1.0f };
		flinearcolor Text_Shadow{ 0.0f, 0.0f, 0.0f, 1.0f };
		flinearcolor Text_Outline{ 0.0f, 0.0f, 0.0f, 1.0f };
		flinearcolor Text_Active = Accent::Red;

		flinearcolor Button_Idle = RGBtoFLC(20, 20, 24);
		flinearcolor Button_Hovered = RGBtoFLC(28, 28, 34);
		flinearcolor Button_Active = Accent::Red;

		flinearcolor Checkbox_Idle = RGBtoFLC(13, 13, 16);
		flinearcolor Checkbox_Hovered = RGBtoFLC(20, 20, 26);
		flinearcolor Checkbox_Enabled = Accent::Red;

		flinearcolor Combobox_Idle = RGBtoFLC(13, 13, 16);
		flinearcolor Combobox_Hovered = RGBtoFLC(20, 20, 26);
		flinearcolor Combobox_Elements = RGBtoFLC(16, 16, 20);

		flinearcolor Slider_Idle = RGBtoFLC(37, 37, 48);
		flinearcolor Slider_Hovered = RGBtoFLC(42, 42, 52);
		flinearcolor Slider_Progress = Accent::Red;
		flinearcolor Slider_Button = Accent::Bright;

		flinearcolor ColorPicker_Background{ 0.006f, 0.006f, 0.006f, 0.4f };
	}

	namespace PostRenderer
	{
		struct DrawList
		{
			int type = -1; //1 = FilledRect, 2 = TextLeft, 3 = TextCenter, 4 = Draw_Line, 5 = Text, 6 = FilledRectEx
			fvector2d pos;
			fvector2d size;
			flinearcolor color;
			const wchar_t* name;
			bool outline;

			fvector2d from;
			fvector2d to;
			int thickness;

			// For new Text function
			fvector2d scale;
			flinearcolor outline_color;
			fvector2d shadow_offset;
			bool center;

			// For new FilledRect function
			float width;
			float height;
		};
		DrawList drawlist[128];

		void drawFilledRect(fvector2d pos, float w, float h, flinearcolor color) // OLD
		{
			for (int i = 0; i < 128; i++)
			{
				if (drawlist[i].type == -1)
				{
					drawlist[i].type = 1;
					drawlist[i].pos = pos;
					drawlist[i].size = fvector2d{ w, h };
					drawlist[i].color = color;
					return;
				}
			}
		}

		void FilledRect(fvector2d pos, float w, float h, flinearcolor color) // NEW
		{
			for (int i = 0; i < 128; i++)
			{
				if (drawlist[i].type == -1)
				{
					drawlist[i].type = 6;
					drawlist[i].pos = pos;
					drawlist[i].width = w;
					drawlist[i].height = h;
					drawlist[i].color = color;
					return;
				}
			}
		}

		void TextLeft(const wchar_t* name, fvector2d pos, flinearcolor color, bool outline) // OLD
		{
			for (int i = 0; i < 128; i++)
			{
				if (drawlist[i].type == -1)
				{
					drawlist[i].type = 2;
					drawlist[i].name = name;
					drawlist[i].pos = pos;
					drawlist[i].outline = outline;
					drawlist[i].color = color;
					return;
				}
			}
		}

		void TextCenter(const wchar_t* name, fvector2d pos, flinearcolor color, bool outline) // OLD
		{
			for (int i = 0; i < 128; i++)
			{
				if (drawlist[i].type == -1)
				{
					drawlist[i].type = 3;
					drawlist[i].name = name;
					drawlist[i].pos = pos;
					drawlist[i].outline = outline;
					drawlist[i].color = color;
					return;
				}
			}
		}

		void Draw_Line(fvector2d from, fvector2d to, int thickness, flinearcolor color) // OLD
		{
			for (int i = 0; i < 128; i++)
			{
				if (drawlist[i].type == -1)
				{
					drawlist[i].type = 4;
					drawlist[i].from = from;
					drawlist[i].to = to;
					drawlist[i].thickness = thickness;
					drawlist[i].color = color;
					return;
				}
			}
		}

		void Text(const wchar_t* text, fvector2d pos, fvector2d scale, flinearcolor color, // NEW
			float angle, flinearcolor shadow_color, fvector2d shadow_offset,
			bool center, bool outline, flinearcolor outline_color)
		{
			for (int i = 0; i < 128; i++)
			{
				if (drawlist[i].type == -1)
				{
					drawlist[i].type = 5;
					drawlist[i].name = text;
					drawlist[i].pos = pos;
					drawlist[i].scale = scale;
					drawlist[i].color = color;
					drawlist[i].outline_color = outline_color;
					drawlist[i].shadow_offset = shadow_offset;
					drawlist[i].center = center;
					drawlist[i].outline = outline;
					return;
				}
			}
		}
	}

	ucanvas* canvas;


	bool hover_element = false;
	fvector2d menu_pos = fvector2d{ 0, 0 };
	fvector2d menu_size = fvector2d{ 0, 0 };
	// Bottom status strip (separator + panel + 1px accent); ConfigDock draws on top of this band.
	inline constexpr float FOOTER_H = 18.f;
	float offset_x = 0.0f;
	float offset_y = 0.0f;

	fvector2d first_element_pos = fvector2d{ 0, 0 };

	fvector2d last_element_pos = fvector2d{ 0, 0 };
	fvector2d last_element_size = fvector2d{ 0, 0 };

	int current_element = -1;
	fvector2d current_element_pos = fvector2d{ 0, 0 };
	fvector2d current_element_size = fvector2d{ 0, 0 };
	int elements_count = 0;

	bool sameLine = false;

	bool pushY = false;
	float pushYvalue = 0.0f;

	void SetupCanvas(ucanvas* _canvas)
	{
		canvas = _canvas;
	}

	void Draw_Line(fvector2d from, fvector2d to, int thickness, flinearcolor color)
	{
		canvas->k2_drawline(fvector2d{ from.x, from.y }, fvector2d{ to.x, to.y }, thickness, color);
	}
		void drawFilledRect(fvector2d initial_pos, float w, float h, flinearcolor color)
	{
		for (float i = 0.0f; i < h; i += 1.0f)
			canvas->k2_drawline(fvector2d{ initial_pos.x, initial_pos.y + i }, fvector2d{ initial_pos.x + w, initial_pos.y + i }, 1.0f, color);
	}

	// Proper alpha fill via WhiteSquareTexture + translucent blend (line-stack fill looks opaque).
	void drawFilledRectTransparent(fvector2d pos, float w, float h, flinearcolor color)
	{
		if (!canvas || w <= 0.f || h <= 0.f) return;
		canvas->K2_DrawTransparentLine(
			fvector2d{ pos.x, pos.y },
			fvector2d{ pos.x + w, pos.y + h },
			1.0f,
			color);
	}

	void drawFilledRect(fvector2d pos, fvector2d size, flinearcolor color)
	{
		drawFilledRect(pos, size.x, size.y, color);
	}
	void DrawGradientLine(fvector2d screenpos_a, fvector2d screenpos_b, flinearcolor color_a, flinearcolor color_c, flinearcolor color_b, float thickness, int num_segments)
	{
		for (int i = 0; i < num_segments; i++)
		{
			float t1 = static_cast<float>(i) / num_segments;
			float t2 = static_cast<float>(i + 1) / num_segments;

			fvector2d start = screenpos_a + (screenpos_b - screenpos_a) * t1;
			fvector2d end = screenpos_a + (screenpos_b - screenpos_a) * t2;

			float t_mid = (t1 + t2) / 2.0f;

			flinearcolor color;

			if (t_mid <= 0.5f)
			{
				float u = 2.0f * t_mid;  // Maps t_mid [0, 0.5] to u [0, 1]
				color.r = color_a.r * (1.0f - u) + color_c.r * u;
				color.g = color_a.g * (1.0f - u) + color_c.g * u;
				color.b = color_a.b * (1.0f - u) + color_c.b * u;
				color.a = color_a.a * (1.0f - u) + color_c.a * u;
			}
			else
			{
				float v = 2.0f * (t_mid - 0.5f);  // Maps t_mid [0.5, 1] to v [0, 1]
				color.r = color_c.r * (1.0f - v) + color_b.r * v;
				color.g = color_c.g * (1.0f - v) + color_b.g * v;
				color.b = color_c.b * (1.0f - v) + color_b.b * v;
				color.a = color_c.a * (1.0f - v) + color_b.a * v;
			}

			canvas->k2_drawline(start, end, thickness, color);
		}
	}

	// DrawGradientLine(startPos, endPos, red, green, blue, 2.0f, 20);
	void drawGradientFilledRect(fvector2d initial_pos, float w, float h, flinearcolor color_a, flinearcolor color_c, flinearcolor color_b, int num_segments)
	{
		for (float i = 0.0f; i < h; i += 1.0f)
			DrawGradientLine(fvector2d{ initial_pos.x, initial_pos.y + i }, fvector2d{ initial_pos.x + w, initial_pos.y + i }, color_a, color_c, color_b, 1.0f, num_segments);
	}

	void drawGradientFilledRectVertical(fvector2d initial_pos, float w, float h, flinearcolor color_a, flinearcolor color_c, flinearcolor color_b, int num_segments)
	{
		for (float i = 0.0f; i < w; i += 1.0f)
			DrawGradientLine(fvector2d{ initial_pos.x + i, initial_pos.y }, fvector2d{ initial_pos.x + i, initial_pos.y + h }, color_a, color_c, color_b, 1.0f, num_segments);
	}


	void draw_filled_rect(ucanvas* canvas, float x, float y, float width, float height, flinearcolor color) {
		for (float i = 0; i < height; i++) {
			canvas->k2_drawline(
				{ x, y + i },
				{ x + width, y + i },
				1.0f,
				color
			);
		}
	}

	void draw_rect(ucanvas* canvas, float x, float y, float width, float height, flinearcolor color) {
		canvas->k2_drawline({ x, y }, { x + width, y }, 1.0f, color);
		canvas->k2_drawline({ x + width, y }, { x + width, y + height }, 1.0f, color);
		canvas->k2_drawline({ x + width, y + height }, { x, y + height }, 1.0f, color);
		canvas->k2_drawline({ x, y + height }, { x, y }, 1.0f, color);
	}

	fvector2d CursorPos()
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		return fvector2d{ (float)cursorPos.x, (float)cursorPos.y };
	}
	bool MouseInZone(fvector2d pos, fvector2d size)
	{
		fvector2d cursor_pos = CursorPos();

		if (cursor_pos.x > pos.x && cursor_pos.y > pos.y)
			if (cursor_pos.x < pos.x + size.x && cursor_pos.y < pos.y + size.y)
				return true;

		return false;
	}

	void drawTriangle(fvector2d p1, fvector2d p2, fvector2d p3, flinearcolor color, int thickness = 1)
	{
		Draw_Line(p1, p2, thickness, color);
		Draw_Line(p2, p3, thickness, color);
		Draw_Line(p3, p1, thickness, color);
	}
	void drawCircle(fvector2d center, float radius, flinearcolor color, int thickness = 1, int segments = 64)
	{
		float angleStep = (2.0f * 3.14159265f) / segments;

		for (int i = 0; i < segments; ++i)
		{
			float angle1 = i * angleStep;
			float angle2 = (i + 1) * angleStep;

			fvector2d p1 = fvector2d(center.x + cosf(angle1) * radius, center.y + sinf(angle1) * radius);
			fvector2d p2 = fvector2d(center.x + cosf(angle2) * radius, center.y + sinf(angle2) * radius);

			Draw_Line(p1, p2, thickness, color);
		}
	}

	//void DrawFilledTriangle(const fvector2d& p1, const fvector2d& p2, const fvector2d& p3, const flinearcolor& color)
	//{
	//    std::vector<fvector2d> verts = { p1, p2, p3 };
	//    std::sort(verts.begin(), verts.end(), [](const fvector2d& a, const fvector2d& b) {
	//        return a.y < b.y;
	//        });

	//    const fvector2d& v0 = verts[0];
	//    const fvector2d& v1 = verts[1];
	//    const fvector2d& v2 = verts[2];

	//    auto interp = [](float a, float b, float t) {
	//        return a + (b - a) * t;
	//        };

	//    float dy1 = v1.y - v0.y;
	//    float dy2 = v2.y - v0.y;
	//    float dy3 = v2.y - v1.y;

	//    for (int y = static_cast<int>(v0.y); y <= static_cast<int>(v2.y); y++) {
	//        if (y < v1.y && dy1 != 0) {
	//            float t1 = (y - v0.y) / dy1;
	//            float t2 = (y - v0.y) / dy2;

	//            float xa = interp(v0.x, v1.x, t1);
	//            float xb = interp(v0.x, v2.x, t2);
	//            if (xa > xb) std::swap(xa, xb);
	//            Draw_Line(fvector2d(xa, y), fvector2d(xb, y), 1.0f, color);
	//        }
	//        else if (dy3 != 0) {
	//            float t1 = (y - v1.y) / dy3;
	//            float t2 = (y - v0.y) / dy2;

	//            float xa = interp(v1.x, v2.x, t1);
	//            float xb = interp(v0.x, v2.x, t2);
	//            if (xa > xb) std::swap(xa, xb);
	//            Draw_Line(fvector2d(xa, y), fvector2d(xb, y), 1.0f, color);
	//        }
	//    }
	//}

	//bool Draw_Cursor(bool draw)
	//{
	//    if (!draw)
	//        return false;

	//    fvector2d mousePos = CursorPos();

	//    // Fixed size
	//    float size = 8.0f;
	//    float height = size * 2.0f;

	//    fvector2d tip = mousePos;
	//    fvector2d left = fvector2d(mousePos.x - size, mousePos.y + height);
	//    fvector2d right = fvector2d(mousePos.x + size, mousePos.y + height);

	//    // Time for fluid animation
	//    float t = static_cast<float>(GetTickCount64()) / 1000.0f;

	//    // Animated triangle fill with per-line brightness shift (fake fluid)
	//    std::vector<fvector2d> verts = { tip, left, right };
	//    std::sort(verts.begin(), verts.end(), [](const fvector2d& a, const fvector2d& b) {
	//        return a.y < b.y;
	//        });

	//    const fvector2d& v0 = verts[0];
	//    const fvector2d& v1 = verts[1];
	//    const fvector2d& v2 = verts[2];

	//    auto interp = [](float a, float b, float t) {
	//        return a + (b - a) * t;
	//        };

	//    float dy1 = v1.y - v0.y;
	//    float dy2 = v2.y - v0.y;
	//    float dy3 = v2.y - v1.y;

	//    for (int y = static_cast<int>(v0.y); y <= static_cast<int>(v2.y); y++) {
	//        float pulse = 0.5f + 0.5f * sinf(t * 2.0f); // slow smooth global pulse
	//        flinearcolor fluidColor = Accent::accent_color_new;
	//        fluidColor.a = pulse;

	//        if (y < v1.y && dy1 != 0) {
	//            float t1 = (y - v0.y) / dy1;
	//            float t2 = (y - v0.y) / dy2;

	//            float xa = interp(v0.x, v1.x, t1);
	//            float xb = interp(v0.x, v2.x, t2);
	//            if (xa > xb) std::swap(xa, xb);
	//            Draw_Line(fvector2d(xa, y), fvector2d(xb, y), 1.0f, fluidColor);
	//        }
	//        else if (dy3 != 0) {
	//            float t1 = (y - v1.y) / dy3;
	//            float t2 = (y - v0.y) / dy2;

	//            float xa = interp(v1.x, v2.x, t1);
	//            float xb = interp(v0.x, v2.x, t2);
	//            if (xa > xb) std::swap(xa, xb);
	//            Draw_Line(fvector2d(xa, y), fvector2d(xb, y), 1.0f, fluidColor);
	//        }
	//    }

	//    return true;
	//}

	void SameLine()
	{
		sameLine = true;
	}
	void PushNextElementY(float y, bool from_last_element = true)
	{
		pushY = true;
		if (from_last_element)
			pushYvalue = last_element_pos.y + last_element_size.y + y;
		else
			pushYvalue = y;
	}
	void NextColumn(float x)
	{
		offset_x = x;
		PushNextElementY(first_element_pos.y, false);
	}
	void ClearFirstPos()
	{
		first_element_pos = fvector2d{ 0, 0 };
	}

	void xDrawTextRGB(const wchar_t* text, float x, float y, flinearcolor color)
	{
		canvas->k2_drawtext(font, text, { x, y }, { 0.85f, 0.85f }, flinearcolor(1, 1, 1, 1), 0.f, { 0, 0, 0, 1 }, { 0, 0 }, 0, 1, 0, { 0, 0, 0, 1 });
	}
	void yDrawTextRGB(const wchar_t* text, float x, float y, flinearcolor color)
	{
		canvas->k2_drawtext(font, text, { x, y }, { 0.85f, 0.85f }, flinearcolor(1, 1, 1, 1), 0.f, { 0, 0, 0, 1 }, { 0, 0 }, 1, 1, 0, { 0, 0, 0, 1 });
	}
	void TextLeft(const wchar_t* name, fvector2d pos, flinearcolor color, bool outline)
	{
		xDrawTextRGB(name, pos.x, pos.y, { 1,1,1,1 });
	}
	void TextCenter(const wchar_t* name, fvector2d pos, flinearcolor color, bool outline, bool kekw = 0)
	{
		if (kekw)
			yDrawTextRGB(name, pos.x, pos.y - 10, color);
		else
			yDrawTextRGB(name, pos.x, pos.y - 10, { 1.f,1.f,1.f,1.0f });
	}

	void GetColor(flinearcolor* color, float* r, float* g, float* b, float* a)
	{
		*r = color->r;
		*g = color->g;
		*b = color->b;
		*a = color->a;
	}
	UINT32 GetColorUINT(int r, int g, int b, int a)
	{
		UINT32 result = (BYTE(a) << 24) + (BYTE(r) << 16) + (BYTE(g) << 8) + BYTE(b);
		return result;
	}
	void DrawFilledCircle(fvector2d pos, float r, flinearcolor color)
	{
		float smooth = 0.07f;

		double PI = 3.14159265359;
		int size = (int)(2.0f * PI / smooth) + 1;

		float angle = 0.0f;
		int i = 0;

		for (; angle < 2 * PI; angle += smooth, i++)
		{
			Draw_Line(fvector2d{ pos.x, pos.y }, fvector2d{ pos.x + cosf(angle) * r, pos.y + sinf(angle) * r }, 1.0f, color);
		}
	}
	void DrawCircle(fvector2d pos, int radius, int numSides, flinearcolor Color)
	{
		float PI = 3.1415927f;

		float Step = PI * 2.0 / numSides;
		int Count = 0;
		fvector2d V[128];
		for (float a = 0; a < PI * 2.0; a += Step) {
			float X1 = radius * cos(a) + pos.x;
			float Y1 = radius * sin(a) + pos.y;
			float X2 = radius * cos(a + Step) + pos.x;
			float Y2 = radius * sin(a + Step) + pos.y;
			V[Count].x = X1;
			V[Count].y = Y1;
			V[Count + 1].x = X2;
			V[Count + 1].y = Y2;
			//Draw_Line(fvector2d{ pos.x, pos.y }, fvector2d{ X2, Y2 }, 1.0f, Color); // Points from Centre to ends of circle
			Draw_Line(fvector2d{ V[Count].x, V[Count].y }, fvector2d{ X2, Y2 }, 1.0f, Color);// Circle Around
		}
	}

	fvector2d dragPos;

	float GetSmoothTime() {
		return static_cast<float>(GetTickCount64()) / 1000.0f;
	}

	flinearcolor ApplyHueShift(flinearcolor color, float shift)
	{
		// Convert RGB to HSV
		float r = color.r;
		float g = color.g;
		float b = color.b;

		float max = max(r, max(g, b));
		float min = min(r, min(g, b));
		float delta = max - min;

		float h = 0.0f;
		float s = (max > 0.0001f) ? delta / max : 0.0f;
		float v = max;

		if (delta > 0.0001f) {
			if (max == r) {
				h = (g - b) / delta;
			}
			else if (max == g) {
				h = 2.0f + (b - r) / delta;
			}
			else {
				h = 4.0f + (r - g) / delta;
			}
			h *= 60.0f;
			if (h < 0.0f) h += 360.0f;
		}

		// Apply hue shift
		h += shift * 180.0f / 3.14159265f; // Convert radians to degrees
		while (h >= 360.0f) h -= 360.0f;
		while (h < 0.0f) h += 360.0f;

		// Convert HSV back to RGB
		float c = v * s;
		float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
		float m = v - c;

		flinearcolor result;
		if (h < 60.0f) {
			result = { c, x, 0.0f };
		}
		else if (h < 120.0f) {
			result = { x, c, 0.0f };
		}
		else if (h < 180.0f) {
			result = { 0.0f, c, x };
		}
		else if (h < 240.0f) {
			result = { 0.0f, x, c };
		}
		else if (h < 300.0f) {
			result = { x, 0.0f, c };
		}
		else {
			result = { c, 0.0f, x };
		}

		result.r += m;
		result.g += m;
		result.b += m;
		result.a = color.a; // Preserve original alpha

		return result;
	}

	inline float smoothstep(float edge0, float edge1, float x)
	{
		// Clamp x to range [0, 1] relative to [edge0, edge1]
		float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		// Cubic Hermite interpolation
		return t * t * (3.0f - 2.0f * t);
	}

	int current_tab;
	bool config_popup_open = false;
	bool footer_ctxstore_popup_open = false;
	bool config_name_edit_active = false;
	std::vector<std::string> config_files_cache;
	ULONGLONG last_config_scan_tick = 0;

	void RefreshConfigFileCache();

	void ForceRefreshConfigFileCache()
	{
		RefreshConfigFileCache();
		last_config_scan_tick = 0;
	}

	void RefreshConfigFileCache()
	{
		config_files_cache.clear();
		std::filesystem::path config_dir = "C:/Cortex";
		if (!std::filesystem::exists(config_dir)) return;

		for (const auto& entry : std::filesystem::directory_iterator(config_dir))
		{
			if (!entry.is_regular_file()) continue;
			const auto ext = entry.path().extension().string();
			if (ext == ".json" || ext == ".cfg" || ext.empty())
				config_files_cache.push_back(entry.path().filename().string());
		}

		std::sort(config_files_cache.begin(), config_files_cache.end());
	}

	bool Window(const wchar_t* name, fvector2d* pos, fvector2d size, bool isOpen)
	{
		elements_count = 0;

		if (!isOpen)
			return false;

		float screen_width = (float)GetSystemMetrics(SM_CXSCREEN);
		float screen_height = (float)GetSystemMetrics(SM_CYSCREEN);

		// Drag from the full top header bar (move menu around)
		const float drag_h = 30.0f;
		bool isHovered = MouseInZone(fvector2d{ pos->x, pos->y }, fvector2d{ size.x, drag_h });

		// Drag
		if ((isHovered || dragPos.x != 0) && !hover_element)
		{
			if (input::is_mouse_clicked(0, elements_count, true))
			{
				fvector2d cursorPos = CursorPos();
				if (dragPos.x == 0)
				{
					dragPos.x = (cursorPos.x - pos->x);
					dragPos.y = (cursorPos.y - pos->y);
				}

				pos->x = cursorPos.x - dragPos.x;
				pos->y = cursorPos.y - dragPos.y;

				if (pos->x < 0) pos->x = 0;
				if (pos->y < 0) pos->y = 0;
				if (pos->x + size.x > screen_width) pos->x = screen_width - size.x;
				if (pos->y + size.y > screen_height) pos->y = screen_height - size.y;
			}
			else
			{
				dragPos = fvector2d{ 0, 0 };
			}
		}

		hover_element = false;

		menu_pos = *pos;
		menu_size = size;
		offset_x = 0; offset_y = 0;
		first_element_pos = fvector2d{ 0, 0 };
		current_element_pos = fvector2d{ 0, 0 };
		current_element_size = fvector2d{ 0, 0 };

		// Soft black glass — a bit darker
		drawFilledRectTransparent(
			fvector2d(pos->x - 2.f, pos->y - 2.f),
			size.x + 4.f, size.y + 4.f,
			flinearcolor(0.f, 0.f, 0.f, 0.14f));
		drawFilledRectTransparent(*pos, size.x, size.y, flinearcolor(0.f, 0.f, 0.f, 0.42f));

		return true;
	}

	void ConfigDock(std::string* selected_config_name)
	{
		if (!selected_config_name) return;
		if (GetTickCount64() - last_config_scan_tick > 1500) {
			RefreshConfigFileCache();
			last_config_scan_tick = GetTickCount64();
		}

		const float fy = menu_pos.y + menu_size.y - FOOTER_H;
		const flinearcolor ico_col = RGBtoFLC(120.f, 120.f, 120.f);
		const float iy = fy + (FOOTER_H - 11.f) / 2.f;

		// Folder — left; opens config popup.
		const fvector2d folder_pos = { menu_pos.x + 8.f, iy };
		const fvector2d folder_size = { 11.f, 10.f };
		const bool folder_hov = MouseInZone(folder_pos, folder_size);
		const flinearcolor folder_face = folder_hov ? RGBtoFLC(140.f, 140.f, 140.f) : ico_col;
		drawFilledRect({ folder_pos.x, folder_pos.y + 2.f }, 11.f, 8.f, folder_face);
		drawFilledRect({ folder_pos.x, folder_pos.y }, 5.f, 3.f, folder_face);

		// Gear (right) — opens Discord ctxstore popup.
		const float gear_cx = menu_pos.x + menu_size.x - 13.f;
		const float gear_cy = fy + FOOTER_H / 2.f;
		const fvector2d gear_hit = { gear_cx - 7.f, gear_cy - 7.f };
		const fvector2d gear_hit_sz = { 14.f, 14.f };
		const bool gear_hov = MouseInZone(gear_hit, gear_hit_sz);
		const flinearcolor gear_col = gear_hov ? RGBtoFLC(168.f, 168.f, 168.f) : ico_col;
		DrawCircle({ gear_cx, gear_cy }, 5, 14, gear_col);

		const float ctx_popup_w = 230.f;
		const float ctx_popup_h = 48.f;
		float ctx_popup_x = gear_cx - ctx_popup_w / 2.f;
		if (ctx_popup_x < menu_pos.x + 6.f)
			ctx_popup_x = menu_pos.x + 6.f;
		if (ctx_popup_x + ctx_popup_w > menu_pos.x + menu_size.x - 6.f)
			ctx_popup_x = menu_pos.x + menu_size.x - 6.f - ctx_popup_w;
		const fvector2d ctx_popup_pos = { ctx_popup_x, fy - 4.f - ctx_popup_h };
		const fvector2d ctx_popup_size = { ctx_popup_w, ctx_popup_h };

		elements_count++;
		if (folder_hov && input::is_mouse_clicked(0, elements_count, false)) {
			config_popup_open = !config_popup_open;
			if (config_popup_open)
				footer_ctxstore_popup_open = false;
		}

		elements_count++;
		if (gear_hov && input::is_mouse_clicked(0, elements_count, false)) {
			footer_ctxstore_popup_open = !footer_ctxstore_popup_open;
			if (footer_ctxstore_popup_open)
				config_popup_open = false;
		}

		if (footer_ctxstore_popup_open) {
			hover_element = true;
			drawFilledRect(ctx_popup_pos, ctx_popup_size.x, ctx_popup_size.y, RGBtoFLC(8, 8, 10));
			draw_rect(canvas, ctx_popup_pos.x, ctx_popup_pos.y, ctx_popup_size.x, ctx_popup_size.y, Accent::Border);
			canvas->k2_drawtext(font, L"discord.gg/ctxstore",
				{ ctx_popup_pos.x + ctx_popup_size.x / 2.f, ctx_popup_pos.y + ctx_popup_size.y / 2.f + 1.f },
				{ 0.74f, 0.74f }, Accent::Text_Dim,
				0.f, { 0, 0, 0, 1 }, { 0, 0 }, true, true, false, { 0, 0, 0, 1 });
		}

		if (!config_popup_open) {
			if (input::is_mouse_clicked(0, 0xC0FFEE, false)) {
				if (footer_ctxstore_popup_open && !MouseInZone(ctx_popup_pos, ctx_popup_size) && !gear_hov && !folder_hov)
					footer_ctxstore_popup_open = false;
			}
			return;
		}

		hover_element = true;
		const float popup_h = 188.f;
		fvector2d popup_pos = fvector2d{ menu_pos.x + 10.0f, fy - 2.f - popup_h };
		fvector2d popup_size = fvector2d{ 230.0f, popup_h };
		drawFilledRect(popup_pos, popup_size.x, popup_size.y, RGBtoFLC(8, 8, 10));
		draw_rect(canvas, popup_pos.x, popup_pos.y, popup_size.x, popup_size.y, Accent::Border);

		canvas->k2_drawtext(font, L"Load cfg", fvector2d(popup_pos.x + 8, popup_pos.y + 12), fvector2d(0.8f, 0.8f), Colors::Text, 0.f, { 0,0,0,1 }, { 0,0 }, false, true, false, { 0,0,0,1 });
		Draw_Line(fvector2d(popup_pos.x + 8, popup_pos.y + 22), fvector2d(popup_pos.x + popup_size.x - 8, popup_pos.y + 22), 1, Accent::Border);

		float row_y = popup_pos.y + 32.0f;
		const float row_h = 18.0f;
		const int max_rows = 5;
		for (int i = 0; i < (int)config_files_cache.size() && i < max_rows; i++) {
			fvector2d row_pos = fvector2d{ popup_pos.x + 8, row_y + (row_h * i) };
			fvector2d row_size = fvector2d{ popup_size.x - 16, row_h };
			bool row_hovered = MouseInZone(row_pos, row_size);
			bool selected = (*selected_config_name == config_files_cache[i]);

			if (row_hovered || selected) {
				drawFilledRect(row_pos, row_size.x, row_size.y, selected ? RGBtoFLC(40, 40, 46) : RGBtoFLC(20, 20, 24));
			}

			elements_count++;
			if (row_hovered && input::is_mouse_clicked(0, elements_count, false)) {
				*selected_config_name = config_files_cache[i];
				try {
					if (Config) Config->LoadSettings(std::string("C:/Cortex/") + *selected_config_name);
				}
				catch (...) {}
			}

			std::wstring wname(config_files_cache[i].begin(), config_files_cache[i].end());
			canvas->k2_drawtext(font, wname.c_str(), fvector2d(row_pos.x + 6, row_pos.y + 9), fvector2d(0.75f, 0.75f), Colors::Text, 0.f, { 0,0,0,1 }, { 0,0 }, false, true, false, { 0,0,0,1 });
		}

		// Editable config name box
		fvector2d name_box_pos = fvector2d{ popup_pos.x + 8, popup_pos.y + popup_size.y - 64 };
		fvector2d name_box_size = fvector2d{ popup_size.x - 16, 18 };
		bool name_box_hovered = MouseInZone(name_box_pos, name_box_size);
		drawFilledRect(name_box_pos, name_box_size.x, name_box_size.y, RGBtoFLC(0, 0, 0));
		draw_rect(canvas, name_box_pos.x, name_box_pos.y, name_box_size.x, name_box_size.y, config_name_edit_active ? Accent::Red : Accent::Border);
		std::wstring wsel(selected_config_name->begin(), selected_config_name->end());
		canvas->k2_drawtext(font, wsel.c_str(), fvector2d(name_box_pos.x + 6, name_box_pos.y + 9), fvector2d(0.70f, 0.70f), Colors::Text, 0.f, { 0,0,0,1 }, { 0,0 }, false, true, false, { 0,0,0,1 });

		elements_count++;
		if (name_box_hovered && input::is_mouse_clicked(0, elements_count, false))
			config_name_edit_active = true;

		if (config_name_edit_active) {
			BYTE kb[256];
			GetKeyboardState(kb);
			for (int vk = 0; vk < 255; vk++) {
				if (!(GetAsyncKeyState(vk) & 0x1)) continue;
				if (vk == VK_BACK) {
					if (!selected_config_name->empty()) selected_config_name->pop_back();
				}
				else if (vk == VK_RETURN) {
					config_name_edit_active = false;
				}
				else if (selected_config_name->size() < 64) {
					WCHAR wc[4] = {};
					int r = ToUnicode(vk, MapVirtualKey(vk, MAPVK_VK_TO_VSC), kb, wc, 4, 0);
					if (r > 0 && iswprint(wc[0])) {
						char mb[4] = {};
						WideCharToMultiByte(CP_UTF8, 0, wc, r, mb, 3, nullptr, nullptr);
						selected_config_name->append(mb);
					}
				}
			}
		}

		auto normalize_cfg_name = [](std::string in) -> std::string {
			if (in.empty()) in = "config";
			for (char& c : in) {
				if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
					c = '_';
			}
			if (in.find('.') == std::string::npos) in += ".cfg";
			return in;
		};

		fvector2d save_btn_pos = fvector2d{ popup_pos.x + 8, popup_pos.y + popup_size.y - 42 };
		fvector2d save_btn_size = fvector2d{ (popup_size.x - 20) / 2.0f, 18 };
		bool save_hovered = MouseInZone(save_btn_pos, save_btn_size);
		drawFilledRect(save_btn_pos, save_btn_size.x, save_btn_size.y, save_hovered ? RGBtoFLC(26, 26, 30) : RGBtoFLC(16, 16, 20));
		draw_rect(canvas, save_btn_pos.x, save_btn_pos.y, save_btn_size.x, save_btn_size.y, Accent::Border);
		canvas->k2_drawtext(font, L"Save", fvector2d(save_btn_pos.x + save_btn_size.x / 2, save_btn_pos.y + 9), fvector2d(0.72f, 0.72f), Colors::Text, 0.f, { 0,0,0,1 }, { 0,0 }, true, true, false, { 0,0,0,1 });

		elements_count++;
		if (save_hovered && input::is_mouse_clicked(0, elements_count, false)) {
			*selected_config_name = normalize_cfg_name(*selected_config_name);
			try {
				std::filesystem::create_directories("C:/Cortex");
				if (Config) Config->SaveSettings(std::string("C:/Cortex/") + *selected_config_name);
				ForceRefreshConfigFileCache();
			}
			catch (...) {}
		}

		fvector2d load_btn_pos = fvector2d{ save_btn_pos.x + save_btn_size.x + 4, save_btn_pos.y };
		fvector2d load_btn_size = save_btn_size;
		bool load_hovered = MouseInZone(load_btn_pos, load_btn_size);
		drawFilledRect(load_btn_pos, load_btn_size.x, load_btn_size.y, load_hovered ? RGBtoFLC(26, 26, 30) : RGBtoFLC(16, 16, 20));
		draw_rect(canvas, load_btn_pos.x, load_btn_pos.y, load_btn_size.x, load_btn_size.y, Accent::Border);
		canvas->k2_drawtext(font, L"Load", fvector2d(load_btn_pos.x + load_btn_size.x / 2, load_btn_pos.y + 9), fvector2d(0.72f, 0.72f), Colors::Text, 0.f, { 0,0,0,1 }, { 0,0 }, true, true, false, { 0,0,0,1 });

		elements_count++;
		if (load_hovered && input::is_mouse_clicked(0, elements_count, false)) {
			*selected_config_name = normalize_cfg_name(*selected_config_name);
			try {
				if (Config) Config->LoadSettings(std::string("C:/Cortex/") + *selected_config_name);
				ForceRefreshConfigFileCache();
			}
			catch (...) {}
		}

		fvector2d open_btn_pos = fvector2d{ popup_pos.x + 8, popup_pos.y + popup_size.y - 22 };
		fvector2d open_btn_size = fvector2d{ popup_size.x - 16, 18 };
		bool open_hovered = MouseInZone(open_btn_pos, open_btn_size);
		drawFilledRect(open_btn_pos, open_btn_size.x, open_btn_size.y, open_hovered ? RGBtoFLC(26, 26, 30) : RGBtoFLC(16, 16, 20));
		draw_rect(canvas, open_btn_pos.x, open_btn_pos.y, open_btn_size.x, open_btn_size.y, Accent::Border);
		canvas->k2_drawtext(font, L"Open folder", fvector2d(open_btn_pos.x + open_btn_size.x / 2, open_btn_pos.y + 9), fvector2d(0.72f, 0.72f), Colors::Text, 0.f, { 0,0,0,1 }, { 0,0 }, true, true, false, { 0,0,0,1 });

		elements_count++;
		if (open_hovered && input::is_mouse_clicked(0, elements_count, false))
		{
			// Ensure folder exists; if it doesn't, Explorer may open default location.
			try { std::filesystem::create_directories("C:/Cortex"); } catch (...) {}
			ShellExecuteA(nullptr, nullptr, "explorer.exe", "C:\\Cortex", nullptr, SW_SHOWNORMAL);
		}

		bool popup_hovered = MouseInZone(popup_pos, popup_size);
		if (!popup_hovered && !folder_hov && !gear_hov && input::is_mouse_clicked(0, 0xC0FFEE, false))
		{
			config_popup_open = false;
			config_name_edit_active = false;
		}
	}

	bool ButtonTab2(const wchar_t* name, fvector2d size, bool active)
	{
		elements_count++;

		fvector2d pos = fvector2d{ menu_pos.x + offset_x, menu_pos.y + offset_y };

		if (sameLine)
		{
			pos.x = last_element_pos.x + last_element_size.x + 4.0f;
		}

		bool isHovered = MouseInZone(pos, size);
		fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };

		if (isHovered)
			hover_element = true;

		if (active) {
			// Selected category: solid white + black text
			drawFilledRect(pos, size.x, size.y, flinearcolor(1.f, 1.f, 1.f, 1.f));
			canvas->k2_drawtext(font, name, textPos, fvector2d(0.98f, 0.98f), flinearcolor(0.f, 0.f, 0.f, 1.f), 0.0f, {0,0,0,0}, {0,0}, true, true, false, {0,0,0,0});
		} else {
			// Idle category: pure black (+ slight lift on hover)
			const flinearcolor bg = isHovered
				? flinearcolor(0.12f, 0.12f, 0.12f, 1.f)
				: flinearcolor(0.f, 0.f, 0.f, 1.f);
			drawFilledRect(pos, size.x, size.y, bg);
			canvas->k2_drawtext(font, name, textPos, fvector2d(0.96f, 0.96f), flinearcolor(1.f, 1.f, 1.f, 1.f), 0.0f, {0,0,0,0}, {0,0}, true, true, false, {0,0,0,0});
		}

		sameLine = true;
		last_element_pos = pos;
		last_element_size = size;

		if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			return true;

		return false;
	}

	void SectionWrapper(const wchar_t* name, fvector2d size)
	{
		fvector2d pos = fvector2d{ menu_pos.x + offset_x, menu_pos.y + offset_y };

		canvas->k2_drawtext(font, name, fvector2d(pos.x + 2.0f, pos.y + 11.0f), fvector2d(0.82f, 0.82f), flinearcolor(0.92f, 0.92f, 0.94f, 1.f), 0.0f, {0,0,0,0}, {0,0}, false, true, false, {0,0,0,0});
		Draw_Line(fvector2d(pos.x, pos.y + 24.f), fvector2d(pos.x + size.x - 4.f, pos.y + 24.f), 1.f,
			flinearcolor(1.f, 1.f, 1.f, 0.45f));

		offset_y += 30.0f;
		sameLine = false;
	}

		void Checkbox(const wchar_t* name, bool* value, bool risky = false)
	{
		elements_count++;

		float box_size = 15.0f;
		float row_width = 250.0f;
		float row_height = 24.0f;
		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 10, menu_pos.y + offset_y };

		if (sameLine)
		{
			pos.x = last_element_pos.x + last_element_size.x + 20;
			pos.y = last_element_pos.y;
		}

		float box_x = pos.x;
		float box_y = pos.y + (row_height - box_size) / 2;

		bool labelHovered = MouseInZone(fvector2d{ pos.x, pos.y }, fvector2d{ 180.0f, row_height });
		bool boxHovered = MouseInZone(fvector2d{ box_x, box_y }, fvector2d{ box_size, box_size });
		bool isHovered = labelHovered || boxHovered;

		canvas->k2_drawtext(font, name, fvector2d(pos.x + box_size + 8, pos.y + 11), fvector2d(0.90f, 0.90f), isHovered ? flinearcolor(1, 1, 1, 1) : Accent::Text_Main, 0.0f, {0,0,0,1}, {0,0}, false, true, false, {0,0,0,1});

		if (*value) {
			drawFilledRect(fvector2d(box_x, box_y), box_size, box_size, flinearcolor(1.f, 1.f, 1.f, 1.f));
			draw_rect(canvas, box_x, box_y, box_size, box_size, flinearcolor(1.f, 1.f, 1.f, 1.f));
		} else {
			drawFilledRect(fvector2d(box_x, box_y), box_size, box_size, flinearcolor(0.05f, 0.05f, 0.05f, 1.f));
			draw_rect(canvas, box_x, box_y, box_size, box_size, isHovered ? flinearcolor(1.f, 1.f, 1.f, 0.85f) : RGBtoFLC(58, 58, 72));
		}

		if (!sameLine)
			offset_y += row_height;

		last_element_pos = pos;
		last_element_size = fvector2d{ row_width, row_height };
		sameLine = false;

		if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			*value = !*value;
	}

	bool Button(const wchar_t* name)
	{
		elements_count++;

		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 20, menu_pos.y + offset_y };
		float button_width = 120.f;
		float button_height = 23.f;

		bool isHovered = MouseInZone(pos, fvector2d{ button_width, button_height });

		if (!sameLine)
			offset_y += button_height + 10.0f;

		drawFilledRect(pos, button_width, button_height, isHovered ? Colors::Button_Hovered : Colors::Button_Idle);
		draw_rect(canvas, pos.x, pos.y, button_width, button_height, Accent::Border);

		canvas->k2_drawtext(font, name, fvector2d{ pos.x + button_width / 2.0f, pos.y + button_height / 2.0f }, fvector2d(0.74f, 0.74f), Colors::Text, 0.f, {0,0,0,1}, {0,0}, true, true, false, {0,0,0,1});

		sameLine = false;
		last_element_pos = pos;
		last_element_size = fvector2d{ button_width, button_height };

		if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			return true;

		return false;
	}

	bool WideButton(const wchar_t* name, float width, float height = 34.f)
	{
		elements_count++;

		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 10, menu_pos.y + offset_y };
		bool isHovered = MouseInZone(pos, fvector2d{ width, height });

		if (!sameLine)
			offset_y += height + 8.0f;

		// Solid black button (Unlock All / Emote)
		drawFilledRect(pos, width, height, isHovered ? flinearcolor(0.08f, 0.08f, 0.08f, 1.f) : flinearcolor(0.f, 0.f, 0.f, 1.f));
		draw_rect(canvas, pos.x, pos.y, width, height, flinearcolor(1.f, 1.f, 1.f, 0.55f));

		canvas->k2_drawtext(font, name,
			fvector2d{ pos.x + width / 2.0f, pos.y + height / 2.0f },
			fvector2d(0.94f, 0.94f), flinearcolor(1.f, 1.f, 1.f, 1.f), 0.f, {0,0,0,1}, {0,0}, true, true, false, {0,0,0,1});

		sameLine = false;
		last_element_pos = pos;
		last_element_size = fvector2d{ width, height };

		if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			return true;

		return false;
	}

	void SliderFloat(const wchar_t* name, float* value, float min, float max, const char* format)
	{
		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 10, menu_pos.y + offset_y };

		float slider_width = 195.f;
		float slider_height = 4.f;

		canvas->k2_drawtext(font, name, fvector2d(pos.x, pos.y + 2), fvector2d(0.88f, 0.88f), Accent::Text_Main, 0.0f, {0,0,0,1}, {0,0}, false, true, false, {0,0,0,1});

		char buffer[64]; sprintf_s(buffer, format, *value);
		wchar_t wbuf[64]; size_t out; mbstowcs_s(&out, wbuf, buffer, 64);
		canvas->k2_drawtext(font, wbuf, fvector2d(pos.x + slider_width, pos.y + 2), fvector2d(0.88f, 0.88f), flinearcolor(1.f, 1.f, 1.f, 1.f), 0.0f, { 0,0,0,1 }, { 0,0 }, true, true, false, { 0,0,0,1 });

		float bar_y = pos.y + 18;
		bool isHovered = MouseInZone(fvector2d{ pos.x, bar_y - 4 }, fvector2d{ slider_width, 12.0f });

		drawFilledRect(fvector2d(pos.x, bar_y), slider_width, slider_height, RGBtoFLC(37, 37, 48));

		float progress = (*value - min) / (max - min);
		if (progress < 0.f) progress = 0.f; if (progress > 1.f) progress = 1.f;

		drawFilledRect(fvector2d(pos.x, bar_y), slider_width * progress, slider_height, flinearcolor(1.f, 1.f, 1.f, 1.f));

		if (isHovered && GetAsyncKeyState(0x01))
		{
			float fraction = (CursorPos().x - pos.x) / slider_width;
			if (fraction < 0.0f) fraction = 0.0f;
			if (fraction > 1.0f) fraction = 1.0f;
			*value = min + fraction * (max - min);
			if (*value < min) *value = min; if (*value > max) *value = max;
		}

		offset_y += 28;
		last_element_pos = pos;
		last_element_size = fvector2d{ slider_width, 25 };
		sameLine = false;
	}


	void SliderInt(const wchar_t* name, int* value, int min, int max, const char* format)
	{
		float val = (float)*value;
		SliderFloat(name, &val, (float)min, (float)max, format);
		*value = (int)val;
	}

	uintptr_t active_combobox = 0;

	void Combobox(fvector2d size, int* value, ...)
	{
		elements_count++;
		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 10, menu_pos.y + offset_y };
		uintptr_t combo_id = (uintptr_t)value;
		if (combo_id == 0) combo_id = (uintptr_t)elements_count;

		bool isHovered = MouseInZone(fvector2d{ pos.x + 1, pos.y + 1 }, fvector2d{ size.x - 2, size.y - 2 });

		if (!sameLine)
			offset_y += size.y + 8.0f;

		std::vector<const wchar_t*> elements;
		va_list args;
		va_start(args, value);
		while (true) {
			const wchar_t* arg = va_arg(args, const wchar_t*);
			if (arg == NULL) break;
			elements.push_back(arg);
		}
		va_end(args);

		if (*value < 0 || *value >= (int)elements.size()) *value = 0;

		bool isOpen = (active_combobox == combo_id);

		drawFilledRect(pos, size.x, size.y, RGBtoFLC(13, 13, 16));
		draw_rect(canvas, pos.x, pos.y, size.x, size.y, isOpen ? Accent::Red : RGBtoFLC(42, 42, 53));

		canvas->k2_drawtext(font, elements[*value], fvector2d{ pos.x + 8, pos.y + size.y / 2.0f }, fvector2d(0.86f, 0.86f), Accent::Text_Main, 0.f, {0,0,0,1}, {0,0}, false, true, false, {0,0,0,1});

		canvas->k2_drawtext(font, L"\x25BC", fvector2d{ pos.x + size.x - 14, pos.y + size.y / 2.0f }, fvector2d(0.62f, 0.62f), Accent::Bright, 0.f, {0,0,0,1}, {0,0}, true, true, false, {0,0,0,1});

		// Toggle dropdown on click
		if (isHovered && input::is_mouse_clicked(0, elements_count, false))
		{
			if (isOpen)
				active_combobox = 0;
			else
				active_combobox = combo_id;
		}

		// Draw dropdown list if open
		if (isOpen)
		{
			hover_element = true;
			float item_h = size.y;
			float dropdown_y = pos.y + size.y;
			float dropdown_h = item_h * (float)elements.size();

			// Dropdown background
			PostRenderer::drawFilledRect(fvector2d(pos.x, dropdown_y), size.x, dropdown_h, RGBtoFLC(12, 12, 12));
			// Dropdown border
			PostRenderer::Draw_Line(fvector2d(pos.x, dropdown_y), fvector2d(pos.x, dropdown_y + dropdown_h), 1, Accent::Border);
			PostRenderer::Draw_Line(fvector2d(pos.x + size.x, dropdown_y), fvector2d(pos.x + size.x, dropdown_y + dropdown_h), 1, Accent::Border);
			PostRenderer::Draw_Line(fvector2d(pos.x, dropdown_y + dropdown_h), fvector2d(pos.x + size.x, dropdown_y + dropdown_h), 1, Accent::Border);

			for (int i = 0; i < (int)elements.size(); i++)
			{
				fvector2d item_pos = fvector2d{ pos.x, dropdown_y + item_h * i };
				bool item_hovered = MouseInZone(item_pos, fvector2d{ size.x, item_h });
				bool is_selected = (*value == i);

				// Highlight hovered or selected item
				if (item_hovered)
					PostRenderer::drawFilledRect(item_pos, size.x, item_h, RGBtoFLC(30, 30, 30));
				else if (is_selected)
					PostRenderer::drawFilledRect(item_pos, size.x, item_h, RGBtoFLC(20, 20, 20));

				// Item text
				flinearcolor item_color = is_selected ? Accent::Red : (item_hovered ? flinearcolor(1,1,1,1) : flinearcolor(0.7f, 0.7f, 0.7f, 1.0f));
				PostRenderer::TextLeft(elements[i], fvector2d{ item_pos.x + 10, item_pos.y + item_h / 2.0f - 4 }, item_color, false);

				// Select on click
				if (item_hovered && input::is_mouse_clicked(0, elements_count + 100 + i, false))
				{
					*value = i;
					active_combobox = 0;
				}
			}

			// Close if clicked outside
			bool anywhereHovered = MouseInZone(fvector2d{ pos.x, pos.y }, fvector2d{ size.x, size.y + dropdown_h });
			if (!anywhereHovered && input::is_mouse_clicked(0, elements_count + 5000, false))
			{
				active_combobox = 0;
			}
		}

		last_element_pos = pos; last_element_size = size; sameLine = false;
	}

	bool SmallButton(const wchar_t* name, float w, float h)
	{
		fvector2d pos = fvector2d{ menu_pos.x + offset_x, menu_pos.y + offset_y };
		fvector2d size = fvector2d{ w, h };

		if (sameLine) {
			pos.x = last_element_pos.x + last_element_size.x + 5.0f;
			pos.y = last_element_pos.y;
		}

		bool isHovered = MouseInZone(pos, size);

		drawFilledRect(pos, size.x, size.y, isHovered ? Colors::Button_Hovered : Colors::Button_Idle);
		TextCenter(name, fvector2d{ pos.x + size.x / 2.0f, pos.y + size.y / 2.0f + 5 }, Colors::Text, false);

		if (isHovered && input::is_mouse_clicked(0, elements_count++, false)) return true;

		last_element_pos = pos; last_element_size = size; sameLine = false;
		return false;
	}
void InputField(const wchar_t* label, std::string* value, size_t max_len)
	{
		elements_count++;
		const uintptr_t textbox_id = (uintptr_t)value;

		float outer_width = 160.0f;
		float outer_height = 18.0f;
		float inner_width = outer_width - 2.0f;
		float inner_height = outer_height - 2.0f;

		fvector2d padding = { 33, 11 };
		fvector2d pos = fvector2d{ menu_pos.x + padding.x + offset_x,
								   menu_pos.y + padding.y + offset_y };

		if (sameLine) {
			pos.x = last_element_pos.x + last_element_size.x + padding.x;
			pos.y = last_element_pos.y;
		}
		if (pushY) {
			pos.y = pushYvalue;
			pushY = false;
			pushYvalue = 0.0f;
			offset_y = pos.y - menu_pos.y;
		}

		bool isHovered = MouseInZone(pos, fvector2d{ outer_width, outer_height });

		if (!sameLine)
			offset_y += outer_height + padding.y + 4.0f;

		// Label au dessus
		canvas->k2_drawtext(font, label,
			fvector2d{ pos.x, pos.y - 12.f },
			fvector2d(0.85f, 0.80f),
			RGBtoFLC(200, 200, 200, 1.f), 0.f,
			Colors::Text_Shadow, fvector2d(0, 0),
			false, false, false, Colors::Text_Outline);

		// Outer border
		drawFilledRect(fvector2d{ pos.x, pos.y }, outer_width, outer_height,
			(active_textbox == textbox_id)
			? Accent::Red   // active
			: RGBtoFLC(22, 24, 30));   // inactif : dark

		// Inner background
		float inner_x = pos.x + 1.f;
		float inner_y = pos.y + 1.f;
		drawFilledRect(fvector2d{ inner_x, inner_y }, inner_width, inner_height,
			(active_textbox == textbox_id)
			? RGBtoFLC(0, 0, 0)
			: RGBtoFLC(0, 0, 0));
		draw_rect(canvas, pos.x, pos.y, outer_width, outer_height, 
			(active_textbox == textbox_id) ? Accent::Red : Accent::Border);

		// Clamp value a max_len pour eviter crash
		if (value->size() > max_len - 1)
			value->resize(max_len - 1);

		// Render text
		std::wstring wval(value->begin(), value->end());
		canvas->k2_drawtext(font, wval.c_str(),
			fvector2d{ inner_x + 5.f, inner_y + inner_height / 2.f },
			fvector2d(0.90f, 0.85f),
			RGBtoFLC(255, 255, 255, 1.2f), 0.f,
			Colors::Text_Shadow, fvector2d(0, 0),
			false, true, false, Colors::Text_Outline);

		// Curseur clignotant
		if (active_textbox == textbox_id) {
			if ((int)(GetTickCount() / 400) % 2) {
				float cursor_x = inner_x + 5.f + (float)wval.size() * 7.5f;
				canvas->k2_drawtext(font, L"|",
					fvector2d{ cursor_x, inner_y + inner_height / 2.f },
					fvector2d(0.85f, 0.80f),
					Accent::Red, 0.f,
					Colors::Text_Shadow, fvector2d(0, 0),
					false, true, false, Colors::Text_Outline);
			}
		}

		// Focus click
		if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			active_textbox = textbox_id;
		else if (!isHovered && input::is_mouse_clicked(0, elements_count, false))
			active_textbox = 0;

		// Input clavier
		if (active_textbox == textbox_id) {
			hover_element = true;

			BYTE kb[256];
			GetKeyboardState(kb);
			bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

			for (int vk = 0; vk < 255; vk++) {
				if (!(GetAsyncKeyState(vk) & 0x1)) continue;

				// Ctrl+V paste
				if (ctrl && (vk == 'V')) {
					if (OpenClipboard(nullptr)) {
						HANDLE h = GetClipboardData(CF_UNICODETEXT);
						if (h) {
							LPCWSTR ws = (LPCWSTR)GlobalLock(h);
							if (ws) {
								char tmp[512] = {};
								WideCharToMultiByte(CP_UTF8, 0, ws, -1, tmp, 511, nullptr, nullptr);
								size_t space = max_len - value->size() - 1;
								if (space > 0)
									value->append(tmp, min(space, strlen(tmp)));
								GlobalUnlock(h);
							}
						}
						CloseClipboard();
					}
					continue;
				}

				if (vk == VK_BACK) {
					if (!value->empty()) value->pop_back();
				}
				else if (vk == VK_RETURN) {
					active_textbox = 0;
				}
				else if (value->size() < max_len - 1) {
					WCHAR wc[4] = {};
					int r = ToUnicode(vk, MapVirtualKey(vk, MAPVK_VK_TO_VSC), kb, wc, 4, 0);
					if (r > 0 && iswprint(wc[0])) {
						char mb[4] = {};
						WideCharToMultiByte(CP_UTF8, 0, wc, r, mb, 3, nullptr, nullptr);
						value->append(mb);
					}
				}
			}
		}

		sameLine = false;
		last_element_pos = pos;
		last_element_size = fvector2d{ outer_width, outer_height };
		if (first_element_pos.x == 0.f) first_element_pos = pos;
	}

	std::string VirtualKeyCodeToString(UCHAR virtualKey)
	{
		// Return empty string for 0 or invalid keys
		if (virtualKey == 0 || virtualKey == -1)
		{
			return "";
		}

		// Handle mouse buttons first
		switch (virtualKey)
		{
		case VK_LBUTTON: return "MB1";
		case VK_RBUTTON: return "MB2";
		case VK_MBUTTON: return "MB3";
		case VK_XBUTTON1: return "MB4";
		case VK_XBUTTON2: return "MB5";
		}

		// Handle special keys with custom names
		switch (virtualKey)
		{
		case VK_INSERT: return "INS";
		case VK_DELETE: return "DEL";
		case VK_HOME: return "HOME";
		case VK_END: return "END";
		case VK_PRIOR: return "PGUP";
		case VK_NEXT: return "PGDN";
		case VK_LEFT: return "LEFT";
		case VK_RIGHT: return "RIGHT";
		case VK_UP: return "UP";
		case VK_DOWN: return "DOWN";
		case VK_SPACE: return "SPACE";
		case VK_RETURN: return "ENTER";
		case VK_ESCAPE: return "ESC";
		case VK_BACK: return "BACK";
		case VK_TAB: return "TAB";
		case VK_CAPITAL: return "CAPS";
		case VK_NUMLOCK: return "NUMLK";
		case VK_SCROLL: return "SCRLK";
		case VK_PAUSE: return "PAUSE";
		case VK_SNAPSHOT: return "PRTSC";
		case VK_LSHIFT: return "LSHIFT";
		case VK_RSHIFT: return "RSHIFT";
		case VK_LCONTROL: return "LCTRL";
		case VK_RCONTROL: return "RCTRL";
		case VK_LMENU: return "LALT";
		case VK_RMENU: return "RALT";
		case VK_LWIN: return "LWIN";
		case VK_RWIN: return "RWIN";
		case VK_APPS: return "MENU";
		case VK_ADD: return "+";
		case VK_SUBTRACT: return "-";
		case VK_MULTIPLY: return "*";
		case VK_DIVIDE: return "/";
		case VK_DECIMAL: return ".";
		case VK_OEM_PLUS: return "=";
		case VK_OEM_MINUS: return "-";
		case VK_OEM_COMMA: return ",";
		case VK_OEM_PERIOD: return ".";
		case VK_OEM_1: return ";";
		case VK_OEM_2: return "/";
		case VK_OEM_3: return "`";
		case VK_OEM_4: return "[";
		case VK_OEM_5: return "\\";
		case VK_OEM_6: return "]";
		case VK_OEM_7: return "'";
		}

		// Handle number pad keys
		if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_NUMPAD9)
		{
			return "NUM" + std::to_string(virtualKey - VK_NUMPAD0);
		}

		// Handle F keys
		if (virtualKey >= VK_F1 && virtualKey <= VK_F24)
		{
			return "F" + std::to_string(virtualKey - VK_F1 + 1);
		}

		// Handle letter keys (A-Z)
		if (virtualKey >= 'A' && virtualKey <= 'Z')
		{
			return std::string(1, (char)virtualKey);
		}

		// Handle number keys (0-9)
		if (virtualKey >= '0' && virtualKey <= '9')
		{
			return std::string(1, (char)virtualKey);
		}

		// For any other keys, use the default Windows name
		UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
		CHAR szName[128];

		// Set extended flag for certain keys
		switch (virtualKey)
		{
		case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
		case VK_RCONTROL: case VK_RMENU:
		case VK_LWIN: case VK_RWIN: case VK_APPS:
		case VK_PRIOR: case VK_NEXT:
		case VK_END: case VK_HOME:
		case VK_INSERT: case VK_DELETE:
		case VK_DIVIDE:
		case VK_NUMLOCK:
			scanCode |= KF_EXTENDED;
		}

		int result = GetKeyNameTextA(scanCode << 16, szName, 128);

		// Convert the Windows name to uppercase for consistency
		std::string keyName = szName;
		std::transform(keyName.begin(), keyName.end(), keyName.begin(), ::toupper);

		return keyName;
	}

	int active_hotkey = -1;
	bool already_pressed = false;

	int active_hotkey_labeled = -1;
	bool already_pressed_labeled = false;

	void Hotkey(const char* name, fvector2d size, int* key)
	{
		elements_count++;
		fvector2d pos = fvector2d{ menu_pos.x + offset_x, menu_pos.y + offset_y };
		if (sameLine)
		{
			pos.x = last_element_pos.x + last_element_size.x + 5;
			pos.y = last_element_pos.y; 
		}
		if (pushY)
		{
			pos.y = pushYvalue;
			pushY = false;
			pushYvalue = 0.0f;
			offset_y = pos.y - menu_pos.y;
		}
		bool isHovered = MouseInZone(fvector2d{ pos.x - 30, pos.y }, fvector2d(size.x + 30, size.y));

		if (!sameLine)
			offset_y += size.y;

		if (active_hotkey == elements_count)
		{
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText = "[ ... ]";
			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.85f, 0.85f), Accent::Red, 0.0f, Colors::Text_Shadow, fvector2d(0, 0), true, true, false, {0,0,0,1});

			// Wait for key release of MB1 before accepting new keys to prevent accidental assignment
			if (!input::mouseDown[0])
				already_pressed = false;

			if (!already_pressed)
			{
				for (int code = 1; code < 255; code++)
				{
					// Ignore MB1/MB2/MB3 for hotkey assignment usually, or handle carefully
					if (code >= 1 && code <= 2) continue;
					if (GetAsyncKeyState(code) & 0x8000)
					{
						*key = code;
						active_hotkey = -1;
						break;
					}
				}
			}

			// If clicked elsewhere, cancel
			if (input::is_mouse_clicked(0, elements_count, false) && !isHovered)
			{
				active_hotkey = -1;
			}
		}
		else
		{
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText;
			if (*key <= 0)
				displayText = "[ NONE ]";
			else
				displayText = "[ " + VirtualKeyCodeToString(*key) + " ]";

			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.75f, 0.75f), isHovered ? flinearcolor(1, 1, 1, 1) : flinearcolor(0.6f, 0.6f, 0.6f, 1.0f), 0.0f, Colors::Text_Shadow, fvector2d(0, 0), true, true, false, {0,0,0,1});

			if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			{
				already_pressed = true;
				active_hotkey = elements_count;
			}
		}

		sameLine = false;
		last_element_pos = pos;
		last_element_size = size;
		if (first_element_pos.x == 0.0f)
			first_element_pos = pos;
	}

	void FieldLabel(const wchar_t* text)
	{
		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 10, menu_pos.y + offset_y };
		canvas->k2_drawtext(font, text, fvector2d(pos.x, pos.y + 4), fvector2d(0.64f, 0.64f), Accent::Text_Dim, 0.0f, {0,0,0,1}, {0,0}, false, true, false, {0,0,0,1});
		offset_y += 18.0f;
		sameLine = false;
	}

	void KeybindRow(const wchar_t* label, fvector2d row_size, int* key)
	{
		elements_count++;
		fvector2d pos = fvector2d{ menu_pos.x + offset_x + 10, menu_pos.y + offset_y };
		float box_w = 50.0f;
		float box_h = 20.0f;
		float box_x = pos.x + row_size.x - box_w;
		float box_y = pos.y + (row_size.y - box_h) / 2;

		canvas->k2_drawtext(font, label, fvector2d(pos.x, pos.y + 10), fvector2d(0.88f, 0.88f), Accent::Text_Main, 0.0f, {0,0,0,1}, {0,0}, false, true, false, {0,0,0,1});

		bool isHovered = MouseInZone(fvector2d{ box_x, box_y }, fvector2d{ box_w, box_h });

		drawFilledRect(fvector2d(box_x, box_y), box_w, box_h, RGBtoFLC(13, 13, 16));
		draw_rect(canvas, box_x, box_y, box_w, box_h, isHovered ? Accent::Red : RGBtoFLC(42, 42, 53));

		fvector2d textPos = fvector2d{ box_x + box_w / 2, box_y + box_h / 2 };

		if (active_hotkey == elements_count)
		{
			canvas->k2_drawtext(font, L"...", textPos, fvector2d(0.84f, 0.84f), Accent::Bright, 0.0f, {0,0,0,1}, {0,0}, true, true, false, {0,0,0,1});
			if (!input::mouseDown[0]) already_pressed = false;
			if (!already_pressed) {
				for (int code = 1; code < 255; code++) {
					if (code >= 1 && code <= 2) continue;
					if (GetAsyncKeyState(code) & 0x8000) { *key = code; active_hotkey = -1; break; }
				}
			}
			if (input::is_mouse_clicked(0, elements_count, false) && !isHovered) active_hotkey = -1;
		}
		else
		{
			std::string displayText = (*key <= 0) ? "NONE" : VirtualKeyCodeToString(*key);
			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.82f, 0.82f), Accent::Bright, 0.0f, {0,0,0,1}, {0,0}, true, true, false, {0,0,0,1});
			if (isHovered && input::is_mouse_clicked(0, elements_count, false)) { already_pressed = true; active_hotkey = elements_count; }
		}

		offset_y += row_size.y + 2;
		last_element_pos = pos;
		last_element_size = row_size;
		sameLine = false;
	}

	void HotkeyBoxed(const char* name, fvector2d size, int* key)
	{
		elements_count++;
		fvector2d pos = fvector2d{ menu_pos.x + offset_x, menu_pos.y + offset_y };
		if (sameLine)
		{
			pos.x = menu_pos.x + offset_x + 135; 
			pos.y = last_element_pos.y + 1; 
		}
		if (pushY)
		{
			pos.y = pushYvalue;
			pushY = false;
			pushYvalue = 0.0f;
			offset_y = pos.y - menu_pos.y;
		}
		bool isHovered = MouseInZone(fvector2d{ pos.x, pos.y }, size);

		if (!sameLine)
			offset_y += size.y;

		drawFilledRect(pos, size.x, size.y, RGBtoFLC(0, 0, 0));
		draw_rect(canvas, pos.x, pos.y, size.x, size.y, Accent::Red);

		if (active_hotkey == elements_count)
		{
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText = "...";
			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.85f, 0.85f), Accent::Red, 0.0f, Colors::Text_Shadow, fvector2d(0, 0), true, true, false, {0,0,0,1});

			if (!input::mouseDown[0])
				already_pressed = false;

			if (!already_pressed)
			{
				for (int code = 1; code < 255; code++)
				{
					if (code >= 1 && code <= 2) continue;
					if (GetAsyncKeyState(code) & 0x8000)
					{
						*key = code;
						active_hotkey = -1;
						break;
					}
				}
			}

			if (input::is_mouse_clicked(0, elements_count, false) && !isHovered)
			{
				active_hotkey = -1;
			}
		}
		else
		{
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText;
			if (*key <= 0)
				displayText = "NONE";
			else
				displayText = VirtualKeyCodeToString(*key);

			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.75f, 0.75f), isHovered ? flinearcolor(1, 1, 1, 1) : flinearcolor(0.6f, 0.6f, 0.6f, 1.0f), 0.0f, Colors::Text_Shadow, fvector2d(0, 0), true, true, false, {0,0,0,1});

			if (isHovered && input::is_mouse_clicked(0, elements_count, false))
			{
				already_pressed = true;
				active_hotkey = elements_count;
			}
		}

		sameLine = false;
		last_element_pos = pos;
		last_element_size = size;
		if (first_element_pos.x == 0.0f)
			first_element_pos = pos;
	}

	void HotkeyLabeled(const char* name, int* key)
	{
		elements_count++;
		fvector2d padding = fvector2d{ 158, -2 };
		fvector2d size = fvector2d{ 9, 18 };
		fvector2d pos = fvector2d{ menu_pos.x + padding.x + offset_x, menu_pos.y + padding.y + offset_y };

		if (sameLine)
		{
			pos.x = last_element_pos.x + last_element_size.x + padding.x;
			pos.y = last_element_pos.y + (last_element_size.y / 2) - size.y / 2;
		}
		if (pushY)
		{
			pos.y = pushYvalue;
			pushY = false;
			pushYvalue = 0.0f;
			offset_y = pos.y - menu_pos.y;
		}

		bool isHovered = MouseInZone(fvector2d{ pos.x, pos.y }, size);

		if (!sameLine)
			offset_y += size.y + padding.y;

		// Label a gauche de la hotkey
		fvector2d labelPos = fvector2d{ pos.x - 10, pos.y + size.y / 2 };
		canvas->k2_drawtext(font, s2wc(name), labelPos,
			fvector2d(0.80f, 0.75f),
			RGBtoFLC(120, 120, 120, 1.2f),
			0.0f, RGBtoFLC(0, 0, 0), fvector2d(0, 0),
			true, true, true, RGBtoFLC(0, 0, 0, 0.1f));

		if (active_hotkey_labeled == elements_count)
		{
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText = "[ ... ]";
			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.80f, 0.75f), RGBtoFLC(120, 120, 120, 1.2f), 0.0f, Colors::Text_Shadow, fvector2d(0, 0), true, true, true, RGBtoFLC(0, 0, 0, 0.1f));

			if (!menu::input::is_any_mouse_down())
				already_pressed_labeled = false;

			if (!already_pressed_labeled)
			{
				for (int code = 0; code < 255; code++)
				{
					if (GetAsyncKeyState(code))
					{
						*key = code;
						active_hotkey_labeled = -1;
					}
				}
			}
		}
		else
		{
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText;
			if (*key == 0 || *key == -1)
				displayText = "[ - ]";
			else
				displayText = "[ " + VirtualKeyCodeToString(*key) + " ]";

			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.80f, 0.75f), RGBtoFLC(120, 120, 120, 1.2f), 0.0f, RGBtoFLC(0, 0, 0), fvector2d(0, 0), true, true, true, RGBtoFLC(0, 0, 0, 0.1f));

			if (isHovered)
			{
				if (input::is_mouse_clicked(0, elements_count, false))
				{
					already_pressed_labeled = true;
					active_hotkey_labeled = elements_count;
					for (int code = 0; code < 255; code++)
						if (GetAsyncKeyState(code)) {}
				}
			}
			else
			{
				if (input::is_mouse_clicked(0, elements_count, false))
					active_hotkey_labeled = -1;
			}
		}

		sameLine = false;
		last_element_pos = pos;
		last_element_size = size;
		if (first_element_pos.x == 0.0f)
			first_element_pos = pos;
	}


	int active_hotkey2 = -1;
	bool already_pressed2 = false;
	void Hotkey2(const char* name, fvector2d size, int* key)
	{
		elements_count++;
		fvector2d padding = fvector2d{ 191, 4 };
		fvector2d pos = fvector2d{ menu_pos.x + padding.x + offset_x, menu_pos.y + padding.y + offset_y };
		if (sameLine)
		{
			pos.x = last_element_pos.x + last_element_size.x + padding.x;
			pos.y = last_element_pos.y + (last_element_size.y / 2) - size.y / 2;
		}
		if (pushY)
		{
			pos.y = pushYvalue;
			pushY = false;
			pushYvalue = 0.0f;
			offset_y = pos.y - menu_pos.y;
		}
		bool isHovered = MouseInZone(fvector2d{ pos.x, pos.y }, size);

		if (!sameLine)
			offset_y += size.y + padding.y;

		if (active_hotkey2 == elements_count)
		{
			//Text
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText = "[ ... ]";

			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.80f, 0.75f), RGBtoFLC(150, 150, 150, 1.2f), 0.0f, Colors::Text_Shadow, fvector2d(0, 0), true, true, true, RGBtoFLC(0, 0, 0, 0.1f));

			if (!menu::input::is_any_mouse_down())
			{
				already_pressed2 = false;
			}

			if (!already_pressed2)
			{
				for (int code = 0; code < 255; code++)
				{
					if (GetAsyncKeyState(code))
					{
						*key = code;
						active_hotkey2 = -1;
					}
				}
			}
		}
		else
		{
			//Text
			fvector2d textPos = fvector2d{ pos.x + size.x / 2, pos.y + size.y / 2 };
			std::string displayText;
			if (*key == 0 || *key == -1) // Check if key is not set
			{
				displayText = "[ - ]";
			}
			else
			{
				displayText = "[ " + VirtualKeyCodeToString(*key) + " ]";
			}

			canvas->k2_drawtext(font, s2wc(displayText.c_str()), textPos, fvector2d(0.80f, 0.75f), RGBtoFLC(150, 150, 150, 1.2f), 0.0f, RGBtoFLC(0, 0, 0), fvector2d(0, 0), true, true, true, RGBtoFLC(0, 0, 0, 0.1f));

			if (isHovered)
			{
				if (input::is_mouse_clicked(0, elements_count, false))
				{
					already_pressed2 = true;
					active_hotkey2 = elements_count;

					//Queue Fix
					for (int code = 0; code < 255; code++)
						if (GetAsyncKeyState(code)) {}
				}
			}
			else
			{
				if (input::is_mouse_clicked(0, elements_count, false))
				{
					active_hotkey2 = -1;
				}
			}
		}

		sameLine = false;
		last_element_pos = pos;
		last_element_size = size;
		if (first_element_pos.x == 0.0f)
			first_element_pos = pos;
	}

	void Render()
	{
		for (int i = 0; i < 128; i++)
		{
			if (PostRenderer::drawlist[i].type != -1)
			{
				// OLD: FilledRect
				if (PostRenderer::drawlist[i].type == 1)
				{
					menu::drawFilledRect(PostRenderer::drawlist[i].pos,
						PostRenderer::drawlist[i].size.x,
						PostRenderer::drawlist[i].size.y,
						PostRenderer::drawlist[i].color);
				}
				// OLD: TextLeft
				else if (PostRenderer::drawlist[i].type == 2)
				{
					canvas->k2_drawtext(font, PostRenderer::drawlist[i].name,
						PostRenderer::drawlist[i].pos,
						fvector2d(0.98, 0.98),
						PostRenderer::drawlist[i].color,
						0.0f, Colors::Text_Shadow,
						fvector2d(0, 0), false, false,
						PostRenderer::drawlist[i].outline, Colors::Text_Outline);
				}
				// OLD: TextCenter
				else if (PostRenderer::drawlist[i].type == 3)
				{
					menu::TextCenter(PostRenderer::drawlist[i].name,
						PostRenderer::drawlist[i].pos,
						PostRenderer::drawlist[i].color,
						PostRenderer::drawlist[i].outline);
				}
				// OLD: Draw_Line
				else if (PostRenderer::drawlist[i].type == 4)
				{
					Draw_Line(PostRenderer::drawlist[i].from,
						PostRenderer::drawlist[i].to,
						PostRenderer::drawlist[i].thickness,
						PostRenderer::drawlist[i].color);
				}
				// NEW: Text (with full parameters)
				else if (PostRenderer::drawlist[i].type == 5)
				{
					canvas->k2_drawtext(font,
						PostRenderer::drawlist[i].name,
						PostRenderer::drawlist[i].pos,
						PostRenderer::drawlist[i].scale,
						PostRenderer::drawlist[i].color,
						0.0f,
						PostRenderer::drawlist[i].outline_color,
						PostRenderer::drawlist[i].shadow_offset,
						PostRenderer::drawlist[i].center,
						true,
						PostRenderer::drawlist[i].outline,
						Colors::Text_Outline);
				}
				// NEW: FilledRectEx
				else if (PostRenderer::drawlist[i].type == 6)
				{
					menu::drawFilledRect(PostRenderer::drawlist[i].pos,
						PostRenderer::drawlist[i].width,
						PostRenderer::drawlist[i].height,
						PostRenderer::drawlist[i].color);
				}

				PostRenderer::drawlist[i].type = -1;
			}
		}
	}
}



