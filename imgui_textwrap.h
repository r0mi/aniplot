// Elmo Trolla, 2019
// License: pick one - public domain / UNLICENSE (https://www.unlicense.org) / MIT (https://opensource.org/licenses/MIT).

// NB! do not repalce these with "#pragma once"
#ifndef IMGUI_TEXTWRAP_H
#define IMGUI_TEXTWRAP_H

//
// Makes some simple uses of imgui text rendering api a bit simpler.
// (text positioning, foreground/background colors)
//
// this is a header-file library.
//    include as usual, but in one file:
//        #define IMGUI_TEXTWRAP_IMPLEMENTATION
//        #include "imgui_textwrap.h"
//

#include "imgui.h"
#include "imgui_internal.h"


class ImguiTextwrap
{
public:

	ImguiTextwrap();
	virtual ~ImguiTextwrap() {};

	void init(ImFont* font, ImDrawList* drawlist=NULL);

	// return string size in pixels
	ImVec2 size(const char* text);

	void set_fgcolor(const ImVec4& color);
	void set_bgcolor(const ImVec4& color);

	// top

	void drawtl(const char* text, float x, float y);
	void drawtr(const char* text, float x, float y);
	void drawtm(const char* text, float x, float y);

	// bottom

	void drawbl(const char* text, float x, float y);
	void drawbr(const char* text, float x, float y);
	void drawbm(const char* text, float x, float y);

	// middle

	void drawml(const char* text, float x, float y);
	void drawmr(const char* text, float x, float y);
	void drawmm(const char* text, float x, float y);

	// baseline

	void drawbll(const char* text, float x, float y);
	void drawblr(const char* text, float x, float y);
	void drawblm(const char* text, float x, float y);

	// if fgcolor or bgcolor or z are None, previous values are used.
	// assumes some kind of pixel-projection..
	// you almost never want to use this method directly.
	void draw(const char* text, float x, float y, int positioning);

	// readonly variables
	float height;
	ImVec4 fgcolor;
	ImVec4 bgcolor;
	ImFont* font;
	ImDrawList* drawlist;

private:

	void m_fix_pos(float x, float y, float w, int positioning, float* x_fix, float* y_fix);
};

#endif // IMGUI_TEXTWRAP_H


#ifdef IMGUI_TEXTWRAP_IMPLEMENTATION

ImguiTextwrap::ImguiTextwrap()
{
	this->font = NULL;
	this->drawlist = NULL;
	this->height = 0.;
	this->fgcolor = ImVec4(1., 1., 1., 1.);
	this->bgcolor = ImVec4(1., 1., 1., 0.);
}


// --------------------------------------------------------------------------
// ---- METHODS -------------------------------------------------------------
// --------------------------------------------------------------------------


void ImguiTextwrap::init(ImFont* font, ImDrawList* drawlist) {
	IM_ASSERT(font);
	this->font = font;
	this->height = font->FontSize;
	this->drawlist = drawlist;
}

// Calculate text size. Text can be multi-line.
ImVec2 ImguiTextwrap::size(const char* text)
{
	IM_ASSERT(this->font);
	ImVec2 text_size = this->font->CalcTextSizeA(this->font->FontSize, FLT_MAX, 0, text, NULL, NULL);
	//if (text_size.x > 0.0f)
	//    text_size.x -= character_spacing_x;
	return text_size;
}


void ImguiTextwrap::set_fgcolor(const ImVec4& color)
{
	fgcolor = color;
}


void ImguiTextwrap::set_bgcolor(const ImVec4& color)
{
	bgcolor = color;
}

// top

void ImguiTextwrap::drawtl(const char* text, float x, float y) { draw(text, x, y, 1+16); }
void ImguiTextwrap::drawtr(const char* text, float x, float y) { draw(text, x, y, 2+16); }
void ImguiTextwrap::drawtm(const char* text, float x, float y) { draw(text, x, y, 4+16); }

// bottom

void ImguiTextwrap::drawbl(const char* text, float x, float y) { draw(text, x, y, 1+32); }
void ImguiTextwrap::drawbr(const char* text, float x, float y) { draw(text, x, y, 2+32); }
void ImguiTextwrap::drawbm(const char* text, float x, float y) { draw(text, x, y, 4+32); }

// middle

void ImguiTextwrap::drawml(const char* text, float x, float y) { draw(text, x, y, 1+64); }
void ImguiTextwrap::drawmr(const char* text, float x, float y) { draw(text, x, y, 2+64); }
void ImguiTextwrap::drawmm(const char* text, float x, float y) { draw(text, x, y, 4+64); }

// baseline

void ImguiTextwrap::drawbll(const char* text, float x, float y) { draw(text, x, y, 1+8); }
void ImguiTextwrap::drawblr(const char* text, float x, float y) { draw(text, x, y, 3+8); }
void ImguiTextwrap::drawblm(const char* text, float x, float y) { draw(text, x, y, 4+8); }


void ImguiTextwrap::draw(const char* text, float x, float y, int positioning)
{
	IM_ASSERT(this->font);
	ImVec2 str_size = this->size(text);
	// position the text according to given flags. top-left, bottom-right, ..
	m_fix_pos(x, y, str_size.x, positioning, &x, &y);

	if (str_size.x > 0) {
		x = floor(x);
		y = floor(y);
		ImDrawList *dl = (this->drawlist != NULL) ? this->drawlist : ImGui::GetWindowDrawList();
		if (dl) {
			// draw the background
			if (bgcolor.w != 0) {
				dl->AddRectFilled(ImVec2(x, y), ImVec2(x + str_size.x + 1, y + this->font->FontSize),
				                  ImColor(bgcolor));
			}
			// draw the text
			dl->AddText(this->font, this->font->FontSize, ImVec2(x, y), ImColor(fgcolor), text, NULL, 0, NULL);
		}
	}
}


// --------------------------------------------------------------------------
// ---- PRIVATE -------------------------------------------------------------
// --------------------------------------------------------------------------


void ImguiTextwrap::m_fix_pos(float x, float y, float w, int positioning, float* x_fix, float* y_fix)
{
	IM_ASSERT(this->font);
	// positioning
	// first 3 bits - left-right-middle
	// next  4 bits - baseline-top-bottom-middle

	if (positioning) {
		if      (positioning & 1) {}            // left
		else if (positioning & 2) x -= w;       // right
		else if (positioning & 4) x -= w / 2.f; // middle

		if      (positioning & 8)  y -= this->font->Ascent;  // baseline
		else if (positioning & 16) {}                        // top
		else if (positioning & 32) y -= this->height;        // bottom
		else if (positioning & 64) y -= this->height / 2.f;  // middle
		//else if (positioning & 64) y += this->height / 2.f - this->font->Descent; // middle
	}

	//*x_fix = floor(x + 0.5f); // round(x); // round missing in visual studio express 2008
	//*y_fix = floor(y + 0.5f); // round(y);
	//*x_fix = round(x);
	//*y_fix = round(y);
	*x_fix = x;
	*y_fix = y;
}

#endif // IMGUI_TEXTWRAP_IMPLEMENTATION
