// Copyright (c) 2015, Elmo Trolla
// LICENSE: https://opensource.org/licenses/ISC

#include <cstdlib>
#include <iostream>
#include <vector>
#include <memory>
#include <math.h>

#ifdef _WIN32
	#include <direct.h> // getcwd
	#define getcwd _getcwd
#else
	#include <unistd.h> // getcwd
#endif

#include <GL/gl3w.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include "imgui_impl_sdl_gl3.h"
#include "imgui_internal.h" // for custom graph renderer
#include <SDL.h>

#include "mip_buf_t.h"

#include "imgui_textwrap.h"
//#include "memfile.h"

#undef min
#undef max

using namespace std;

SDL_Window* window = NULL;

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

#define INITIAL_SCREEN_WIDTH 1200
#define INITIAL_SCREEN_HEIGHT 700

#define LOG_IMVEC2(vec) SDL_Log(#vec" %.5f %.5f\n", vec.x, vec.y)
#define LOG_IMRECT(rc)  SDL_Log(#rc" min %.5f %.5f max %.5f %.5f\n", rc.Min.x, rc.Min.y, rc.Max.x, rc.Max.y)
#define LOG_PORTAL(rc)  SDL_Log(#rc" min %.5f %.5f max %.5f %.5f\n", rc.min.x, rc.min.y, rc.max.x, rc.max.y)
#define LOG_AACS(cs)    SDL_Log(#cs" pos %.5f %.5f axis %.5f %.5f\n", cs.pos.x, cs.pos.y, cs.xyaxis.x, cs.xyaxis.y)

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

static inline ImVec2 operator-(const ImVec2& v)                                 { return ImVec2(-v.x, -v.y); } // fdkz:
static inline ImVec2 operator/(const float lhs, const ImVec2& rhs)              { return ImVec2(lhs/rhs.x, lhs/rhs.y); } // fdkz:
static inline ImVec2 operator*(const float lhs, const ImVec2& rhs)              { return ImVec2(lhs*rhs.x, lhs*rhs.y); } // fdkz:
static inline ImVec2& operator*=(ImVec2& lhs, const ImVec2& rhs)                { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; } // fdkz:

#define EPSILON 0.0000001f

struct ImVec2d
{
	double x, y;
	ImVec2d() { x = y = 0.0f; }
	ImVec2d(double _x, double _y) { x = _x; y = _y; }
	ImVec2d(const ImVec2& v) { x = v.x; y = v.y; }

	//const ImVec2 &toImVec2() { return ImVec2(x, y); };
	ImVec2 toImVec2() { return ImVec2(x, y); };
};

static inline ImVec2d operator*(const ImVec2d& lhs, const double rhs)             { return ImVec2d(lhs.x*rhs, lhs.y*rhs); }
static inline ImVec2d operator/(const ImVec2d& lhs, const double rhs)             { return ImVec2d(lhs.x/rhs, lhs.y/rhs); }
static inline ImVec2d operator+(const ImVec2d& lhs, const ImVec2d& rhs)           { return ImVec2d(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2d operator-(const ImVec2d& lhs, const ImVec2d& rhs)           { return ImVec2d(lhs.x-rhs.x, lhs.y-rhs.y); }
static inline ImVec2d operator*(const ImVec2d& lhs, const ImVec2d& rhs)           { return ImVec2d(lhs.x*rhs.x, lhs.y*rhs.y); }
static inline ImVec2d operator/(const ImVec2d& lhs, const ImVec2d& rhs)           { return ImVec2d(lhs.x/rhs.x, lhs.y/rhs.y); }
static inline ImVec2d operator-(const ImVec2d& v)                                 { return ImVec2d(-v.x, -v.y); }
static inline ImVec2d operator/(const double lhs, const ImVec2d& rhs)             { return ImVec2d(lhs/rhs.x, lhs/rhs.y); }
static inline ImVec2d operator*(const double lhs, const ImVec2d& rhs)             { return ImVec2d(lhs*rhs.x, lhs*rhs.y); }
static inline ImVec2d& operator*=(ImVec2d& lhs, const ImVec2d& rhs)               { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
static inline ImVec2d& operator+=(ImVec2d& lhs, const ImVec2d& rhs)               { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2d& operator-=(ImVec2d& lhs, const ImVec2d& rhs)               { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2d& operator*=(ImVec2d& lhs, const double rhs)                 { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2d& operator/=(ImVec2d& lhs, const double rhs)                 { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
//static inline ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs)            { return ImVec4(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-lhs.w); }



//// axis-aligned coordinate system frame. no rotation possible. only scaling and translation.
//struct AACoordSys2d {
//    AACoordSys2d(): xyaxis(1,1) {}
//    AACoordSys2d(const AACoordSys2d& cs): pos(cs.pos), xyaxis(cs.xyaxis) {}
//    AACoordSys2d(const ImVec2& _pos, const ImVec2& _xyaxis): pos(_pos), xyaxis(_xyaxis) {}
//    AACoordSys2d(const ImRect& rect): xyaxis(1./(rect.Max-rect.Min)) {
//        // TODO: think about this a bit more. can proj-in-out to achieve the same result?
//        pos = -xyaxis * rect.Min;
//    }
//    // AACoordSys2d(const ImRect& rect) {
//    //     pos = -rect.Min;
//    //     xyaxis = 1. / (rect.Max - rect.Min);
//    // }
//
//    AACoordSys2d proj_in(const AACoordSys2d& cs)  { return AACoordSys2d(proj_vin(cs.pos), cs.xyaxis / xyaxis); }
//    AACoordSys2d proj_out(const AACoordSys2d& cs) { return AACoordSys2d(proj_vout(cs.pos), cs.xyaxis * xyaxis); }
//
//    // Project the vector from world-space into this coordinate system.
//    inline ImVec2 proj_vin(const ImVec2& v)  { return (v - pos) / xyaxis; }
//    // Project the vector out from this coordinate system into world-space
//    inline ImVec2 proj_vout(const ImVec2& v) { return (v * xyaxis) + pos; }
//
//    ImVec2 pos;
//    ImVec2 xyaxis; // x parameter represents an axis (x, 0) and the y parameter (0, y)
//};

// A rectangle with some helper functions to enable coordinatesystem-like behaviour.
// (min, max) is always mapped to (0, 1) of parent space.
struct PortalRect {
	ImVec2d min;
	ImVec2d max;

	PortalRect()                                          : min(0.,0.), max(1.,1.)   {}
	PortalRect(const ImVec2d& _min, const ImVec2d& _max)  : min(_min), max(_max)     {}
	PortalRect(double x1, double y1, double x2, double y2): min(x1, y1), max(x2, y2) {}
	PortalRect(const ImRect& r) : min(r.Min), max(r.Max) {}

	ImVec2d center()                  const { return ImVec2d((min.x+max.x)*0.5f, (min.y+max.y)*0.5f); }
	ImVec2d size()                    const { return ImVec2d(max.x-min.x, max.y-min.y); }
	double  w()                       const { return max.x-min.x; }
	double  h()                       const { return max.y-min.y; }
	bool   contains(const ImVec2d& p) const { return p.x >= min.x  &&  p.y >= min.y  &&  p.x < max.x  &&  p.y < max.y; }
	//ImRect get_imrect()               const { return ImRect(min.x, min.y, max.x, max.y); } // too unprecise

	void translate(ImVec2d delta) { min += delta; max += delta; }

	// ratio 1: no change. ratio 1.3: 100 -> 130. ratio 1/1.3: 130 -> 100. ratio 2: 100 -> 200, ratio 0.5: 200 -> 100
	void resize_by_ratio(const ImVec2d& _center, const ImVec2d& ratio) { min = _center + (min - _center) * ratio; max = _center + (max - _center) * ratio; }
	void clip(const PortalRect& _clip) { if (min.x < _clip.min.x) min.x = _clip.min.x; if (min.y < _clip.min.y) min.y = _clip.min.y; if (max.x > _clip.max.x) max.x = _clip.max.x; if (max.y > _clip.max.y) max.y = _clip.max.y; }

	PortalRect proj_in(const PortalRect& cs)   const { return PortalRect(proj_vin(cs.min), proj_vin(cs.max)); }
	PortalRect proj_out(const PortalRect& cs)  const { return cs.proj_in(*this); }

	// Project the vector from world-space into this coordinate system. (Coordinates between 0..1 appear inside this rectangle.)
	inline ImVec2d proj_vin(const ImVec2d& v)  const { return v * size() + min; }
	inline ImVec2d proj_vout(const ImVec2d& v) const { return (v - min) / size(); }
};


enum GraphVisualFlags_ {
	GraphVisualFlags_DrawLeftGridLegend       = 1 << 0, // value axis legend
	GraphVisualFlags_DrawRightGridLegend      = 1 << 1, // value axis legend
	GraphVisualFlags_DrawTopGridLegend        = 1 << 2, // time axis legend
	GraphVisualFlags_DrawBottomGridLegend     = 1 << 3, // time axis legend
	GraphVisualFlags_DrawHorizontalGrid       = 1 << 4,
	GraphVisualFlags_DrawVerticalGrid         = 1 << 5,
	GraphVisualFlags_DrawMinMaxBackground     = 1 << 6,
	//GraphVisualFlags_DrawTopScrollbar         = 1 << 7, // scrollbar flags are the only things that shrink the visible graph window size. all other legends are rendered on the graph.
	//GraphVisualFlags_DrawBottomScrollbar      = 1 << 8,
	GraphVisualFlags_GridLegendIsAbsoluteTime = 1 << 9,
	GraphVisualFlags_GridLegendIsRelativeTime = 1 << 10,
};


// Contains data about the graph:
//      MipBuf, name, unit, description
//      value_sample_portal : how to project samples from sample-space to value-space. this object resides in valuespace.
struct GraphChannel {
	MipBuf_t<float> data_channel;
	std::string name; // for the legend
	std::string unit; // displayed on the legend after value
	std::string description;

	// we HAVE to know this to calculate timepoints from sample numbers.
	//float sample_freq;
	// Useful to show data limits.
	float value_min;
	float value_max;
	PortalRect portal; // How to project samples from sample-space to value-space. Resides in sample-space.

	//GraphChannel(float _sample_freq=1000) { set_value_samplespace_mapping(ImRect(0,0, _sample_freq,1)); }
	GraphChannel() { portal.max = ImVec2d(1000., 1.);} // { set_value_samplespace_mapping(ImRect(0,0, 1000,1)); }

	// convenience functions
	inline void append_sample(float v) { data_channel.append(v); }
	inline void append_sample_minmaxavg(float min, float max, float avg) { data_channel.append_minmaxavg(min, max, avg); }

	//void set_portal(PortalRect& _portal) { portal = _portal; } // sample_freq = fabs(_portal.max.x - _portal.min.x);
	void set_value_limits(float min, float max) { value_min = min; value_max = max; }
	// How samplevalues should be mapped to a unit-box in valuespace.
	// ImRect(1, 10,  2, 255) maps samplenum 1 to 0s, sampleval 10 to 0V, samplenum 2 to 1s, sampleval 255 to 1V.
	void set_value_samplespace_mapping(const ImRect& innerspace) { portal = PortalRect(innerspace); }

	// convert coordinates between spaces. for example 1V to raw sample val 255, time 0s to samplenum 0, ..

	// input: (time, value), return: (samplenum, sampleval)
	inline ImVec2d value_to_samplespace(const ImVec2d& value)  const { return portal.proj_vin(value); }
	// input: (samplenum, sampleval), return: (time, value)
	inline ImVec2d sample_to_valuespace(const ImVec2d& sample) const { return portal.proj_vout(sample); }
};


// Contains info about how to draw the data (color, linewidth, background color between min-max values..)
// There can be many GraphVisual objects per GraphChannel, so one GraphChannel can be drawn to many places with many different ways.
struct GraphVisual {
	GraphVisual(GraphChannel* _graph_channel, uint32_t flags=0) {
		IM_ASSERT(_graph_channel);
		graph_channel = _graph_channel;
		line_color = ImColor(200, 200, 200);
		line_color_minmax = ImColor(200, 150, 150, 150);
		minmax_bgcolor = ImColor(20, 20, 20);
		bg_color = ImColor(0, 0, 0);
		hor_grid_color = ImVec4(0.15, 0.15, 0.15, 1.0);
		hor_grid_text_color = ImVec4(1., 1., 1., 0.8);
		hor_grid_text_bgcolor = ImVec4(0.2, 0.2, 0.2, 0.8);
		ver_grid_color = ImVec4(0.15, 0.15, 0.15, 1.0);
		ver_grid_text_color = ImVec4(1., 1., 1., 0.8);
		ver_grid_text_bgcolor = ImVec4(0.2, 0.2, 0.2, 0.8);
		grid_legend_color = ImVec4(0.65, 0.15, 0.15, 1.0);
		grid_min_div_horizontal_pix = 50.0;
		grid_min_div_vertical_pix = 100.0;
		flags = 0;
		anchored = true;
		visible = true;
		// mirror horizontally. pixel coordinates start from top, but we want values start from bottom.
		//portal = PortalRect(0,1, 1,0);
		// also set y zero to center of the window
		portal = PortalRect(0,1, 1,-1);
	}

	GraphChannel* graph_channel;  // GraphVisual does NOT take ownership of the graph_channel.
	ImVec4        line_color;
	ImVec4        line_color_minmax;
	ImVec4        minmax_bgcolor; // background that is filled outside of the value_min/value_max area given in GraphChannel. but is unused if value_min == value_max.
	ImVec4        bg_color;
	ImVec4        hor_grid_color;
	ImVec4        hor_grid_text_color;
	ImVec4        hor_grid_text_bgcolor;
	ImVec4        ver_grid_color;
	ImVec4        ver_grid_text_color;
	ImVec4        ver_grid_text_bgcolor;
	ImVec4        grid_legend_color;
	float         grid_min_div_horizontal_pix;
	float         grid_min_div_vertical_pix;
	float         line_width;     // in pixels
	bool          anchored;       // newest sample is anchored to the right window edge.
	bool          visible;        // graph line rendered or not.
	uint32_t      flags;          // GraphVisualFlags_
	PortalRect    portal;         // Everything coming out of this portal between 0..1 is visible. Resides in value-space.

	// // zoom: visualcenterpos: coordinates on the graph window around which everything changes. usually the coordinates under the mouse. 0..1
	void resize_by_ratio(const ImVec2d& center, const ImVec2d& ratio) { portal.resize_by_ratio(portal.proj_vin(center), ratio); }

	// How should values be mapped to a box with origin 0,0 and size 1,1. 0,0 is top-left corner.
	// innerspace is a rect of values and time.
	// innerspace ex: ((5.6V, 0s), (1.1V, 60s)) - this shows 1 minute worth of data from start of the recording,
	//                bottom of the window starts from 1.1V and top of the window ends with 5.6V).
	void set_visual_valuespace_mapping(const ImRect& innerspace) { portal = PortalRect(innerspace); }
	//ImRect get_visual_valuespace_mapping() { return portal.get_imrect(); }

	// input: (x, y) where visible x and y are 0..1, return: (time, value)
	inline ImVec2d visual_to_valuespace(const ImVec2d& visual)  { return portal.proj_vin(visual); }
	// input: (time, value), return: (x, y) where visible x and y are 0..1
	inline ImVec2d value_to_visualspace(const ImVec2d& value)   { return portal.proj_vout(value); }

	// convenience functions

	// input: (x, y) where visible x and y are 0..1, return: (samplenum, sampleval)
	inline ImVec2d visual_to_samplespace(const ImVec2d& visual) { return graph_channel->portal.proj_vin( portal.proj_vin(visual) ); }
	// input: (samplenum, sampleval), return: (x, y) where visible x and y are 0..1
	inline ImVec2d sample_to_visualspace(const ImVec2d& sample) { return portal.proj_vout( graph_channel->portal.proj_vout(sample) ); }
};


// Can contain many graphs (Graph objects).
// Handles user interaction (mouse dragging, keyboard movement)
// Animates movements smoothly
struct GraphWidget {
	GraphWidget() {
		draw_bottom_scrollbar = false;
		draw_top_scrollbar = false;
		anchored = true;
		m_textrend = new ImguiTextwrap();
	}

	~GraphWidget() {
		delete m_textrend;
	}

	// Does NOT take ownership.
	void add_graph(GraphVisual* graph_visual) {
		graph_visuals.push_back( graph_visual );
	}

	// pos inside area is returned as 0..1
	//ImVec2 _screenpixel_to_visualspace(ImRect area, ImVec2 pos) {}

	// DoSpecialGraph - for example something that renders some buttons, special application-specific ui, .., to the same window.
	//void DoGraph(const ImVec2& size) {}

	// size in pixels, with scrollbars (if scrollbars are turned on)
	void DoGraph(const ImVec2& size) {

		if (!m_textrend->font) m_textrend->init(ImGui::GetWindowFont());

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return;
		if (size.y <= 0 || size.x <= 0) return;
		if (!graph_visuals.size()) return;

		GraphVisual& graph_visual = *graph_visuals[0];
		IM_ASSERT(graph_visual.graph_channel);
		GraphChannel& graph_channel = *graph_visual.graph_channel;

		//const ImRect bb(ImGui::GetWindowPos(), ImGui::GetWindowPos()+ImGui::GetWindowSize());
		//const ImRect bb(window->DC.CursorPos, window->DC.CursorPos+size);
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos+ImGui::GetContentRegionAvail());

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->PushClipRect(ImVec4(bb.Min.x, bb.Min.y, bb.Max.x, bb.Max.y));

		// TODO: this name here might be not the right thing to use
		const ImGuiID id = window->GetID(graph_channel.name.c_str());

		ImGuiState& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		ImGui::ItemSize(bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, &id))
			return;

		PortalRect screen_visualspace_portal(bb);
		ImVec2d visualmousecoord = screen_visualspace_portal.proj_vout(g.IO.MousePos); // mouse coord in graph visualspace. 0,0 is top-left corner of the graph, 1,1 bottom-right.

		//LOG_IMVEC2(visualmousecoord);

		//int v_hovered = -1;
		const bool hovered = ImGui::IsHovered(bb, 0);

		if (hovered) {
			ImGuiIO& io = ImGui::GetIO();
			ImVec2 zoom(0.,0.);

			// if mouse wheel moves, zoom height if ctrl or right mousebutton is down. else zoom width.
			if (io.MouseWheel < 0) {
				zoom = (io.KeyCtrl || g.IO.MouseDown[1]) ? ImVec2(1., 1.3) : ImVec2(1.3, 1.);
			}
			if (io.MouseWheel > 0) {
				zoom = (io.KeyCtrl || g.IO.MouseDown[1]) ? ImVec2(1., 1./1.3) : ImVec2(1./1.3, 1.);
			}
			if (zoom.x || zoom.y) {
				for (int i = 0; i < graph_visuals.size(); i++) {
					// TODO: exp if mousewheel turned more than 1 unit.
					graph_visuals[i]->resize_by_ratio(visualmousecoord, zoom);
				}
			}
		}

		//if (hovered)
		//    g.HoveredId = id;

		if (hovered && g.IO.MouseClicked[0])
		{
			//GImGui->IO.MousePos
			m_dragging = true;
			//GetMouseDragDelta();
		//    SetActiveID(id, window);
		//    FocusWindow(window);
		}

		if (!g.IO.MouseDown[0]) {
			m_dragging = false;
		}

		if (m_dragging) {
			// TODO: floating-point problems. different graphs might drift apart.
			//       correct way to solve it would be to make GraphVisual recursive (be able to contain a list of GraphVisual objects)
			//       and here move only the top-level GraphVisual portal.
			for (int i = 0; i < graph_visuals.size(); i++) {
				graph_visuals[i]->portal.translate( -g.IO.MouseDelta / bb.GetSize() * graph_visuals[i]->portal.size() );
			}
			ImVec2 dragdelta(ImGui::GetMouseDragDelta(0));
			if (dragdelta.x > 0) graph_visual.anchored = false;
		}

		float last_sample_in_visualspace = graph_visual.sample_to_visualspace(ImVec2(graph_channel.data_channel.size(), 0.)).x;
		if (last_sample_in_visualspace < 1) graph_visual.anchored = true;

		if (graph_visual.anchored) {
			// move visualspace so that its right edge is on the last sample
			float last_sample_in_valuespace = graph_channel.sample_to_valuespace(ImVec2(graph_channel.data_channel.size(), 0.)).x;
			float portal_width = graph_visual.portal.w();
			for (int i = 0; i < graph_visuals.size(); i++) {
				graph_visuals[i]->portal.min.x = last_sample_in_valuespace - portal_width;
				graph_visuals[i]->portal.max.x = last_sample_in_valuespace;
			}
		}

		// TODO: limits!

		// calculate a coordinate system screen_sample_portal that converts from sample-space directly to screen-space (pixel-coordinates)

		// TODO: i don't quite know yet why this works. is this inversion of the portal?
		// TODO: make this double. bb hast to be ImVec2d in that case.
		PortalRect screen_visualspace_portal_(-1.0f/bb.GetSize()*bb.Min, -1.0f/bb.GetSize()*bb.Min + 1.0f/bb.GetSize());
		PortalRect screen_sample_portal = graph_visual.portal.proj_out( graph_channel.portal ).proj_in(screen_visualspace_portal_);
		PortalRect screen_value_portal = graph_visual.portal.proj_in(screen_visualspace_portal_);

		// render all graphs, but render grid and text only according to the first graph.

		_render_minmax_background(screen_value_portal, graph_visual, bb);

		_render_grid_horlines(screen_value_portal, graph_visual, bb);
		_render_grid_verlines(screen_value_portal, graph_visual, bb);

		for (int i = 0; i < graph_visuals.size(); i++) {
			if (graph_visuals[i]->visible)
				_draw_graphlines(screen_sample_portal, *graph_visuals[i], bb);
		}

		_render_grid_horlegend(screen_value_portal, graph_visual, bb);
		_render_grid_verlegend(screen_value_portal, graph_visual, bb);

		_render_legend(bb);

		draw_list->PopClipRect();
	}

	void _render_minmax_background(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb) {
		// TODO: coordinates are NOT pixelperfect. does canvas_bb wrap pixels, or pixel edges?
		IM_ASSERT(graph_visual.graph_channel);
		GraphChannel& graph_channel = *graph_visual.graph_channel;
		PortalRect& bb_valuespace = graph_visual.portal;
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		double y1 = screen_value_portal.proj_vout(ImVec2(0., graph_channel.value_min)).y;
		double y2 = screen_value_portal.proj_vout(ImVec2(0., graph_channel.value_max)).y;
		if (y1 > y2) { float y = y1; y1 = y2; y2 = y; }

		if (graph_channel.value_min == graph_channel.value_max) {
			draw_list->AddRectFilled( ImVec2(canvas_bb.Min.x, canvas_bb.Min.y), ImVec2(canvas_bb.Max.x, canvas_bb.Max.y), ImColor(graph_visual.bg_color) );
		} else {
			draw_list->AddRectFilled( ImVec2(canvas_bb.Min.x, canvas_bb.Min.y), ImVec2(canvas_bb.Max.x, y1), ImColor(graph_visual.minmax_bgcolor) );
			draw_list->AddRectFilled( ImVec2(canvas_bb.Min.x, y2), ImVec2(canvas_bb.Max.x, y1), ImColor(graph_visual.bg_color) );
			draw_list->AddRectFilled( ImVec2(canvas_bb.Min.x, y2), ImVec2(canvas_bb.Max.x, canvas_bb.Max.y), ImColor(graph_visual.minmax_bgcolor) );
		}
	}

	// Return value is a number of gridlines to render. One line off the screen on both ends to be able to render half-visible legend.
	// If the return value (num_gridlines) is 0, other values are undefined.
	// min_val is allowed to be larger than max_val and size_pixels is allowed to be negative.
	int _calculate_gridlines(double min_val, double max_val, double size_pixels, double min_pixels_per_div, double* out_val_begin, double* out_val_end, double* out_val_step) {
		if (fabs(size_pixels) < 1. || fabs(max_val - min_val) < EPSILON) {
			out_val_step = 0;
			return 0;
		}
		// using "volt" as a concrete mental shortcut for the value axis. the word "value" has too many meanings.
		double volt_max = MAX(min_val, max_val);
		double volt_min = MIN(min_val, max_val);
		double volts_per_min_div = (volt_max - volt_min) / size_pixels * min_pixels_per_div; // volts per minimum allowed distance between gridlines.
		double volt_step = pow(2.f, ceil(log2(volts_per_min_div))); // calculate a divisible-by-two value near volts_per_min_div. can't have the grid step by irrational numbers..
		double volt_begin = floor(volt_min / volt_step) * volt_step; // volt value of the first gridline to be rendered. volt_min divisible by volt_step.
		double volt_end = ceil(volt_max / volt_step) * volt_step;
		*out_val_begin = volt_begin;
		*out_val_end = volt_end;
		*out_val_step = volt_step;
		return round((volt_end - volt_begin) / volt_step) + 1;
	}

	void _render_grid_horlines(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb) {
		// TODO: coordinates are NOT pixelperfect. does canvas_bb wrap pixels, or pixel edges?
		PortalRect& bb_valuespace = graph_visual.portal;

		// using "volt" as a concrete mental shortcut for the value axis. the word "value" has too many meanings.
		double volt_begin, volt_end, volt_step;
		int num_gridlines = _calculate_gridlines(bb_valuespace.min.y, bb_valuespace.max.y, canvas_bb.GetHeight(), graph_visual.grid_min_div_horizontal_pix, &volt_begin, &volt_end, &volt_step);
		if (volt_step == 0 || volt_end <= volt_begin) return;

		double pixel_y_begin   = screen_value_portal.proj_vout(ImVec2(0., volt_begin)).y;
		double pixels_per_volt = screen_value_portal.proj_vout(ImVec2(0., 1.)).y - screen_value_portal.proj_vout(ImVec2(0., 0.)).y;
		double pixel_y_step    = pixels_per_volt * volt_step;
		//SDL_Log("num_gridlines %d pixels_per_volt %.5f pixel_y_begin %.5f pixel_y_step %.5f\n", num_gridlines, pixels_per_volt, pixel_y_begin, pixel_y_step);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		double y = pixel_y_begin;
		for (int i = 0; i < num_gridlines; i++) {
			draw_list->AddLine(ImVec2(canvas_bb.Min.x, y), ImVec2(canvas_bb.Max.x, y), ImColor(graph_visual.hor_grid_color));
			y += pixel_y_step;
		}
	}

	void _render_grid_horlegend(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb) {
		PortalRect& bb_valuespace = graph_visual.portal;

		// using "volt" as a concrete mental shortcut for the value axis. the word "value" has too many meanings.
		double volt_begin, volt_end, volt_step;
		int num_gridlines = _calculate_gridlines(bb_valuespace.min.y, bb_valuespace.max.y, canvas_bb.GetHeight(), graph_visual.grid_min_div_horizontal_pix, &volt_begin, &volt_end, &volt_step);
		if (volt_step == 0 || volt_end <= volt_begin) return;

		double pixel_y_begin   = screen_value_portal.proj_vout(ImVec2(0., volt_begin)).y;
		double pixels_per_volt = screen_value_portal.proj_vout(ImVec2(0., 1.)).y - screen_value_portal.proj_vout(ImVec2(0., 0.)).y;
		double pixel_y_step    = pixels_per_volt * volt_step;

		this->m_textrend->set_fgcolor(ImColor(graph_visual.hor_grid_text_color));
		this->m_textrend->set_bgcolor(ImColor(graph_visual.hor_grid_text_bgcolor));

		double y = pixel_y_begin;
		double value = volt_begin;
		char txt[20];

		for (int i = 0; i < num_gridlines; i++) {
			ImFormatString(txt, sizeof(txt), "%.2f", value);
			this->m_textrend->drawml(txt, canvas_bb.Min.x, y);
			y += pixel_y_step;
			value += volt_step;
		}

		//this->m_textrend->set_bgcolor(ImColor(graph_visual.hor_grid_text_bgcolor));
		this->m_textrend->drawml(graph_visual.graph_channel->unit.c_str(), canvas_bb.Min.x + 60, canvas_bb.Min.y + 13);
	}

	void _render_grid_verlines(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb) {
		PortalRect& bb_valuespace = graph_visual.portal;

		// using "time" as a concrete mental shortcut for the samplenum axis.
		double time_begin, time_end, time_step;
		int num_gridlines = _calculate_gridlines(bb_valuespace.min.x, bb_valuespace.max.x, canvas_bb.GetWidth(), graph_visual.grid_min_div_vertical_pix, &time_begin, &time_end, &time_step);
		if (time_step == 0 || time_end <= time_begin) return;

		double pixel_x_begin   = screen_value_portal.proj_vout(ImVec2(time_begin, 0.)).x;
		double pixels_per_time = screen_value_portal.proj_vout(ImVec2(1., 0.)).x - screen_value_portal.proj_vout(ImVec2(0., 0.)).x;
		double pixel_x_step    = pixels_per_time * time_step;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		double x = pixel_x_begin;
		for (int i = 0; i < num_gridlines; i++) {
			draw_list->AddLine(ImVec2(x, canvas_bb.Min.y), ImVec2(x, canvas_bb.Max.y), ImColor(graph_visual.ver_grid_color));
			x += pixel_x_step;
		}
	}

	void _render_grid_verlegend(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb) {
		PortalRect& bb_valuespace = graph_visual.portal;

		// using "time" as a concrete mental shortcut for the samplenum axis.
		double time_begin, time_end, time_step;
		int num_gridlines = _calculate_gridlines(bb_valuespace.min.x, bb_valuespace.max.x, canvas_bb.GetWidth(), graph_visual.grid_min_div_vertical_pix, &time_begin, &time_end, &time_step);
		if (time_step == 0 || time_end <= time_begin) return;

		double pixel_x_begin   = screen_value_portal.proj_vout(ImVec2(time_begin, 0.)).x;
		double pixels_per_time = screen_value_portal.proj_vout(ImVec2(1., 0.)).x - screen_value_portal.proj_vout(ImVec2(0., 0.)).x;
		double pixel_x_step    = pixels_per_time * time_step;

		this->m_textrend->set_fgcolor(ImColor(graph_visual.hor_grid_text_color));
		this->m_textrend->set_bgcolor(ImColor(graph_visual.hor_grid_text_bgcolor));

		double x = pixel_x_begin;
		double value = time_begin;
		for (int i = 0; i < num_gridlines; i++) {
			char txt[20];
			_grid_timestr(value, time_step, txt, sizeof(txt));
			this->m_textrend->drawbm(txt, x, canvas_bb.Max.y);
			x += pixel_x_step;
			value += time_step;
		}
	}

	void _render_legend(const ImRect& canvas_bb) {
		int text_h = this->m_textrend->height + 10; // TODO: text height seems to be wrong

		// TODO: how the f does this work? + 4 is for top padding, but we also have the bottom to take care of, yet everything is perfect with 4 instead of 8 here.
		int h = text_h * graph_visuals.size() + 4; // checkbox height with builtin padding
		int w = text_h * 10; // roughly how many characters wide. works only with square fonts
		int x = canvas_bb.Min.x + 80;
		int y = canvas_bb.Max.y - h - 40;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 p_min = ImVec2(x, y);
		ImVec2 p_max = ImVec2(x+w, y+h);

		ImColor fill_col = ImColor(0, 0, 0, 128);
		ImColor border_col = ImColor(170, 170, 170, 128);
		int rounding = 0;

		draw_list->AddRectFilled(p_min, p_max, fill_col, rounding);
		draw_list->AddRect(p_min, p_max, border_col, rounding);

		x += 4; y += 4; // pad

		ImGui::SetCursorPos(ImVec2(x,y));
		ImGui::BeginGroup();

		for (int i = 0; i < graph_visuals.size(); i++) {

			if (!graph_visuals[i]->graph_channel) continue;
			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(graph_visuals[i]->line_color));

			ImGui::Checkbox(graph_visuals[i]->graph_channel->name.c_str(), &graph_visuals[i]->visible);
			ImGui::PopStyleColor(1);
			ImGui::PopID();

			// draw little horizontal line (with current graph color) inside the checkbox if checkbox is not set.
			if (!graph_visuals[i]->visible) {
				float line_y = y + text_h * i + text_h/2 - 1;
				draw_list->AddLine(ImVec2(x + 3, line_y), ImVec2(x + 16, line_y), ImColor(graph_visuals[i]->line_color)); // ImColor(graph_visual.ver_grid_color));
			}
		}
		ImGui::EndGroup();
	}

	void _grid_timestr(double seconds, double step, char* outstr, int outstr_size) {
		double s = fabs(seconds);
		int days = int(floor(s / (60 * 60 * 24)));
		int hours = int(floor(s / (60 * 60))) % 24;
		int minutes = int(floor(s / 60)) % 60;
		float seconds2 = int(s) % 60;

		if (s < 60) {
			ImFormatString(outstr, outstr_size, "%.2fs", s);
		} else if (s < 60. * 60.) {
			ImFormatString(outstr, outstr_size, "%im%.2fs", minutes, seconds2);
		} else if (s < 60. * 60. * 24) {
			ImFormatString(outstr, outstr_size, "%ih%im%.2fs", hours, minutes, seconds2);
		} else {
			ImFormatString(outstr, outstr_size, "%id%ih%im%.2fs", days, hours, minutes, seconds2);
		}

		if (seconds < 0.) {
			memmove(outstr+1, outstr, outstr_size-1);
			outstr[0] = '-';
		}
	}

	// draw the squiggly lines and also the min-max value for every sample if the graph is sufficiently zoomed out.
	// canvas_bb : in screenspace, pixels.
	void _draw_graphlines(const PortalRect& screen_sample_portal, GraphVisual& graph_visual, const ImRect& canvas_bb) {
		IM_ASSERT(graph_visual.graph_channel);
		GraphChannel& graph_channel = *graph_visual.graph_channel;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// get samplenums of graph window edges
		double samplespace_x1 = graph_visual.visual_to_samplespace(ImVec2d(0.,0.)).x;
		double samplespace_x2 = graph_visual.visual_to_samplespace(ImVec2d(1.,0.)).x;

		if (samplespace_x2 == samplespace_x1) return;

		// declare values we get from the top-level MipBuf
		double out_start_pixel, out_start_sample, out_end_pixel, out_end_sample;
		int    out_start_index, out_end_index;
		MipBuf_t<float>* mipbuf;

		graph_channel.data_channel.get_buf(
			samplespace_x1, samplespace_x2, canvas_bb.GetWidth(),
			&mipbuf,
			&out_start_pixel, &out_start_index, &out_start_sample,
			&out_end_pixel, &out_end_index, &out_end_sample);

		if (out_start_index >= out_end_index) return;

		// top-level samples per returned mip-level index.
		float dsample = (out_end_sample - out_start_sample) / (out_end_index - out_start_index);
		float dpixel = (out_end_pixel - out_start_pixel) / (out_end_index - out_start_index);
		// top-level samples per pixel
		float samples_per_pixel = (samplespace_x2 - samplespace_x1) / canvas_bb.GetWidth();
		float pixelx_start = (out_start_sample - screen_sample_portal.min.x) / (screen_sample_portal.max.x - screen_sample_portal.min.x);
		float _screen_sample_portal_height = 1. / (screen_sample_portal.max.y - screen_sample_portal.min.y);

		//
		// Step through current mip-level indices by pixels_per_index pixels, starting from out_start_pixel.
		//

		//ImVec2d p0, p1;
		ImColor linecolor(0,0,0,0);
		//int c;

		// render minmax background line if we don't use the topmost mip level.
		if (mipbuf != &graph_channel.data_channel) {
			linecolor = graph_visual.line_color_minmax;
			//c = 0;
			float x = pixelx_start;
			for (int i = out_start_index; i <= out_end_index; i++) {
				float y1 = (mipbuf->get(i)->minval - (float)screen_sample_portal.min.y) * _screen_sample_portal_height;
				float y2 = (mipbuf->get(i)->maxval - (float)screen_sample_portal.min.y) * _screen_sample_portal_height;
				draw_list->AddLine(ImVec2(x, y1), ImVec2(x, y2), linecolor);
				x += dpixel;

				// slower but shorter method:
				//p0 = screen_sample_portal.proj_vout( ImVec2d(out_start_sample + dsample * c, mipbuf->get(i)->minval) );
				//p1 = screen_sample_portal.proj_vout( ImVec2d(out_start_sample + dsample * c, mipbuf->get(i)->maxval) );
				//draw_list->AddLine(p0.toImVec2(), p1.toImVec2(), linecolor);
				//c++;
			}
		}

		linecolor = graph_visual.line_color;
		//ImVec2 p0, p1;
		//p0 = screen_sample_portal.proj_vout( ImVec2d(out_start_sample, mipbuf->get(out_start_index)->avg) );
		//c = 1;
		float x1, x2, y1, y2;
		x1 = x2 = pixelx_start;
		y1 = (mipbuf->get(out_start_index)->avg - (float)screen_sample_portal.min.y) * _screen_sample_portal_height;

		for (int i = out_start_index + 1; i <= out_end_index; i++) {
			y2 = (mipbuf->get(i)->avg - (float)screen_sample_portal.min.y) * _screen_sample_portal_height;
			x2 += dpixel;
			draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), linecolor);
			x1 = x2; y1 = y2;

			//p1 = screen_sample_portal.proj_vout( ImVec2d(out_start_sample + dsample * c, mipbuf->get(i)->avg) );
			//draw_list->AddLine(p0.toImVec2(), p1.toImVec2(), linecolor);
			//p0 = p1;
			//c++;
		}
	}

	ImVector<GraphVisual*> graph_visuals;

	bool draw_bottom_scrollbar;
	bool draw_top_scrollbar;

	// graph is moving from right to left? newest sample is anchored to the right window edge.
	bool anchored;
	bool m_dragging;

	ImguiTextwrap* m_textrend;
};


void fill_graph_with_testdata(GraphChannel& graph)
{
	for(int i = 0; i < 10000; i++) {
		float v = sin(i*M_PI/180.);
		graph.data_channel.append_minmaxavg(v, v, v);
	}
}

typedef std::shared_ptr<GraphWidget>  GraphWidgetPtr;
typedef std::shared_ptr<GraphChannel> GraphChannelPtr;
typedef std::shared_ptr<GraphVisual>  GraphVisualPtr;



struct GraphWorldStream {
	GraphWorldStream(): stream_id(0) {}
	uint8_t stream_id;
	std::vector<GraphChannelPtr> graph_channels;
	std::vector<GraphVisualPtr>  graph_visuals;
};

// Holds streams.
// A stream can consist of multiple channels.
// Channels can be mapped to different graph_widgets. (well not yet)

class GraphWorld
{
public:
	GraphWorld() { graph_widgets.push_back(GraphWidgetPtr(new GraphWidget)); }

	GraphWorldStream streams[255];
	std::vector<GraphWidgetPtr> graph_widgets;

	inline GraphChannel* get_graph_channel(uint8_t stream_id, uint8_t channel_index_in_stream) {
		// ensure that the wanted channel exists
		_fill_stream_with_channels(stream_id, channel_index_in_stream + 1);
		return streams[stream_id].graph_channels[channel_index_in_stream].get();
	}

	//void clear_stream(uint8_t stream_id) {
	//	// TODO: how??
	//}

	// stream_id : 0..255
	// channel_id : 0..255
	// datatype not implemented
	void update_channel_info(uint8_t stream_id, uint8_t channel_index_in_stream,
			uint8_t* channel_name, uint8_t* channel_unit, uint8_t datatype, float channel_frequency,
			uint32_t rgba,
			float value_min, float value_max,
			float portal_x1, float portal_y1, float portal_x2, float portal_y2) {

		_fill_stream_with_channels(stream_id, channel_index_in_stream + 1);

		GraphWorldStream* stream = &streams[stream_id];

		// TODO: replace the whole channel if datatypes are different

		GraphChannel* channel = stream->graph_channels[channel_index_in_stream].get();
		GraphVisual*  visual  = stream->graph_visuals[channel_index_in_stream].get();

		//visual->set_visual_valuespace_mapping(ImRect(0., 1., 1., -1));
		visual->line_color = ImColor(rgba);
		// How samplevalues should be mapped to a unit-box in valuespace.
		// ImRect(1, 10,  2, 255) maps samplenum 1 to 0s, sampleval 10 to 0V, samplenum 2 to 1s, sampleval 255 to 1V.
		channel->set_value_samplespace_mapping(ImRect(portal_x1, portal_y1, portal_x2, portal_y2));
		channel->set_value_limits(value_min, value_max);
		channel->name = (char*)channel_name;
		channel->unit = (char*)channel_unit;
	}

	// make sure stream contains at least num_channels
	void _fill_stream_with_channels(uint8_t stream_id, uint8_t num_channels) {
		while (streams[stream_id].graph_channels.size() < num_channels) {
			GraphChannelPtr channel = GraphChannelPtr(new GraphChannel);
			GraphVisualPtr  visual  = GraphVisualPtr(new GraphVisual(channel.get()));
			streams[stream_id].graph_channels.push_back(channel);
			streams[stream_id].graph_visuals.push_back(visual);
			graph_widgets[0]->add_graph(visual.get());
		}
	}
};


using namespace std;

#define P_CHANNEL_SAMPLES 10
#define P_CHANNEL_INFO 11

#pragma pack(push,1)
struct p_channel_info {
	uint8_t  packet_type;
	uint8_t  packet_version;
	uint8_t  stream_id;

	uint8_t  channel_index; // channel index in stream. starts from 0.
	uint8_t  channel_name[51]; // zero-terminated
	uint8_t  unit[51];  // zero-terminated
	uint8_t  datatype; // "b", "f", "B", "d", "i", "u", "I", "U", "h", "H"; // only f is supported
	uint8_t  reserved;
	float    frequency;
	uint32_t rgba;
	// used to draw visual limits. if you know your signal is for example 0..5V, use 0 as min and 5 as max here.
	float    value_min;
	float    value_max;
	// used to translate and scale the samples to value-space
	// x1 and y1 is mapped to 0 in value space, x2 and y2 is mapped to 1 in value space.
	// for example using (0, 5,  1000, 0) maps sample num 1000 to 1s and sampleval 1 to 5V.
	float    portal_x1;
	float    portal_y1;
	float    portal_x2;
	float    portal_y2;
};
#pragma pack(pop)

#pragma pack(push,1)
struct p_channel_samples {
	uint8_t  packet_type;
	uint8_t  packet_version;
	uint8_t  stream_id;
	uint8_t  channel_index; // channel index in stream. starts from 0.
	uint8_t  channel_packet_sn;
	uint8_t  datatype; // "b", "f", "B", "d", "i", "u", "I", "U", "h", "H"; // only f is supported
	uint16_t samples_bytes;
	float    samples[];
};
#pragma pack(pop)



#ifdef _WIN32
	//#include <iostream>
	#include <winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h> // sockaddr_in
	#include <arpa/inet.h>  // inet_addr
#endif


class UdpListener {
public:
	#ifdef _WIN32
		WSADATA _wsa_data;
		SOCKET _socket;
		sockaddr_in _address;
	#else
		int _socket;
		sockaddr_in _address;
	#endif // _WIN32

	bool _initialized;

	UdpListener() {
		uint16_t PORT = 59100;

		_initialized = false;

		#ifdef _WIN32

			if (WSAStartup(MAKEWORD(2, 2), &_wsa_data) != NO_ERROR) {
				SDL_Log("UdpListener: error in WSAStartup\n");
				WSACleanup();
				return;
			}

			_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (_socket == INVALID_SOCKET) {
				SDL_Log("UdpListener: error creating socket\n");
				WSACleanup();
				return;
			}

			// TODO: clean to zero
			_address.sin_family = AF_INET;
			_address.sin_addr.s_addr = inet_addr("0.0.0.0");
			_address.sin_port = htons(PORT);

			// If iMode!=0, non-blocking mode is enabled.
			u_long iMode = 1;
			ioctlsocket(_socket, FIONBIO, &iMode);

			if (bind(_socket, (SOCKADDR*)&_address, sizeof(_address)) == SOCKET_ERROR) {
				SDL_Log("UdpListener: failed to bind socket\n");
				WSACleanup();
				return;
			}

		#else

			_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (_socket < 0) {
				SDL_Log("UdpListener: error creating socket\n");
				return;
			}

			// zero out the structure
			memset((char*)&_address, 0, sizeof(_address));

			_address.sin_family = AF_INET;
			_address.sin_addr.s_addr = inet_addr("0.0.0.0");
			_address.sin_port = htons(PORT);

			if (bind(_socket, (struct sockaddr*)&_address, sizeof(_address)) < 0) {
				SDL_Log("UdpListener: failed to bind socket\n");
				return;
			}

		#endif // _WIN32

		_initialized = true;
	}

	int _recvfrom(uint8_t* rxbuf, size_t maxlen) {
		sockaddr_in remote_addr;
		// TODO: need to differentiate between win and others? and the server_length stuff - what's that?
		#ifdef _WIN32
			int server_length = sizeof(struct sockaddr_in);
			return recvfrom(_socket, (char*)rxbuf, maxlen, 0, (SOCKADDR*)&remote_addr, &server_length);
		#else
			socklen_t server_length = sizeof(struct sockaddr_in);
			return recvfrom(_socket, (char*)rxbuf, maxlen, MSG_DONTWAIT, (struct sockaddr*)&remote_addr, &server_length);
		#endif
	}

	void tick(GraphWorld& graph_world) {
		if (!_initialized) return;

		uint8_t rxbuf[65535];

		int n = 1;
		while (n > 0) {
			n = _recvfrom(rxbuf, sizeof(rxbuf));
			if (n <= 0) break;

			uint8_t packet_type = rxbuf[0];

			if (packet_type == P_CHANNEL_SAMPLES && n > sizeof(p_channel_samples)) {

				p_channel_samples* p = (p_channel_samples*)rxbuf;
				if (p->packet_version != 1) continue;

				GraphChannel* graph_channel = graph_world.get_graph_channel(p->stream_id, p->channel_index);
				float* samples = &p->samples[0];
				for (int i = 0; i < p->samples_bytes / 4; i++) {
					float v = samples[i];
					graph_channel->append_sample_minmaxavg(v, v, v);
				}

			} else if (packet_type == P_CHANNEL_INFO) {

				p_channel_info* p = (p_channel_info*)rxbuf;
				if (p->packet_version != 1) continue;

				graph_world.update_channel_info(p->stream_id, p->channel_index,
					p->channel_name, p->unit, p->datatype, p->frequency, p->rgba,
					p->value_min, p->value_max,
					p->portal_x1, p->portal_y1, p->portal_x2, p->portal_y2);
			}
		}

		//sendto(_socket, txbuf, strlen(txbuf), 0, (SOCKADDR*)&remote_Addr, server_length);
	}
};


ImRect get_window_coords() {
	return ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos()+ImGui::GetWindowSize());
	//const ImRect bb(window->DC.CursorPos, window->DC.CursorPos+size);
	//const ImRect bb(window->DC.CursorPos, window->DC.CursorPos+ImGui::GetContentRegionAvail());
}


int main(int, char**)
{
	SDL_Log("initializing SDL\n");

	char dirbuf[10000];
	SDL_Log("wd: %s\n", getcwd(dirbuf, sizeof(dirbuf)));

	//if (SDL_Init(SDL_INIT_VIDEO) != 0) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		SDL_Log("SDL_Init error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	// opengl version 4.1 is max for yosemite. 2015-07-14
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Log("SDL_CreateWindow\n");

	window = SDL_CreateWindow(
			"Telemetry",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			INITIAL_SCREEN_WIDTH,
			INITIAL_SCREEN_HEIGHT,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		SDL_Log("SDL_CreateWindow error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Log("SDL_GL_CreateContext\n");

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (gl_context == NULL) {
		SDL_Log("SDL_GL_CreateContext error: %s\n", SDL_GetError());
		return -1;
	}

	// Initialize GL3W
	int r = gl3wInit();
	if (r != 0) {
		SDL_Log("Error initializing GL3W! %i\n", r);
		return -1;
	}

	SDL_Log("Supported GLSL version is %s.\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Use Vsync, set for late swap tearing.
	if (SDL_GL_SetSwapInterval(-1) < 0) {
		SDL_Log("Warning: Unable to set VSync for late swap tearing! SDL Error: %s\n", SDL_GetError());
	}

	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(window);


	// --------------------------------------------------------------------

	ImVec4 clear_color = ImColor(114, 144, 154);
	ImguiTextwrap textrend;

	GraphWorld graph_world;
	// creates one channel. without it, nothing would be rendered until someone updated some channels.
	GraphChannel* graph_channel = graph_world.get_graph_channel(0, 0);
	graph_channel->name = "default";

	UdpListener windows_udp_listener;

#if 0

	if (0) {
		GraphChannel* graph_channel = graph_world.get_graph_channel(0, 0);

		// TODO: 500 million samples crash on windows. 1.8GB memory gets allocated, then crash.
		for (int i = 0; i < 500000; i++) {
			float v = 0.9*sinf(i * 1000. / 60. / 3. * M_PI / 180.) +
					0.5*sinf(i * 1000. / 60. / 3.333 * M_PI / 180.) +
					0.3*sinf(i * 1000. / 60. / 31. * M_PI / 180.) +
					sinf(i * 1000. / 60. / 321. * M_PI / 180.) +
					sinf(i * 1000. / 60. / 12345. * M_PI / 180.) +
					2.*sinf(i * 1000. / 60. / 567890. * M_PI / 180.) +
					sinf(i * 1000. / 60. / 58890000. * M_PI / 180.);
			graph_channel->append_sample(v);
		}
	}

	if (0) {
		//char* filename = "tek0005both channels.csv-ch1.samples-f32";
		char* filename = "test.samples-f32";
		//char* filename = "tek0005both channels.csv-ch1.samples-f32";
		SDL_Log("opening file '%s'", filename);
		MemFile f(filename);
		if (f.buf) {
			float* buf = (float*)f.buf;
			SDL_Log("file size %i bytes. loading data to the channel object.", f.file_size);
			for (int i = 0; i < f.file_size/4; i++) {
				float v = buf[i]; // TODO: does compiler optimize this to the while-not-last-ptr method?
				graph_channel.data_channel.append_minmaxavg(v, v, v);
			}
			SDL_Log("import done");
		} else {
			SDL_Log("file open failed");
		}
	}
#endif

	// Main loop
	bool done = false;
	while (!done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT) done = true;
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) done = true;
			}
		}
		ImGui_ImplSdlGL3_NewFrame();

		ImGuiIO &io = ImGui::GetIO();

		// make the window always fullsize.
		// disable window background rendering.
		// remove borders, titlebar, menus, ..
		// TODO: change window margins?


		//ImGui::ShowTestWindow();

		if (1) {
			float prev_bgalpha = GImGui->Style.WindowFillAlphaDefault; // PushStyleVar() doesn't seem to support this yet
			GImGui->Style.WindowFillAlphaDefault = 0.;
			ImGui::SetNextWindowSize(io.DisplaySize, ImGuiSetCond_Always);
			ImGui::Begin("Robot", NULL,
						 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
						 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_ShowBorders);
			GImGui->Style.WindowFillAlphaDefault = prev_bgalpha;

			uint32_t milliseconds = SDL_GetTicks();

			//float v = sin(milliseconds / 3. * M_PI / 180.);
			//graph_channel.data_channel.append_minmaxavg(v, v, v);

			windows_udp_listener.tick(graph_world);

			//for (auto graph : graph_world) {}
			graph_world.graph_widgets[0]->DoGraph(ImVec2(1000, 600));

			if (!textrend.font) textrend.init(ImGui::GetWindowFont());

			char txt[50];
			ImFormatString(txt, sizeof(txt), "fps: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
			ImRect windim = get_window_coords();
			textrend.set_bgcolor(ImColor(80,80,80,100));
			textrend.set_fgcolor(ImColor(255,255,255,100));
			textrend.drawtr(txt, windim.Max.x - 14, windim.Min.y + 14);

			ImGui::End();
		}


		if (0) {

			// testing ground

			ImGui::Begin("deb", NULL);

			ImFont *f = ImGui::GetWindowFont();

			float x = floor(1.);
			float y = floor(1.);

			const char* txt = "AB";
			ImVec2 str_size = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0, txt, NULL, NULL);

			float up_left_x = x;
			float up_left_y = y;
			float down_right_x = x + str_size.x + 1;
			float down_right_y = y + f->FontSize;
			// TODO: ascent + descent == size?


			// 1.0 ,1.0  2.0 ,2.0  - this draws 1x1 square one pixel away from the corner
			// 1.5 ,1.5  2.0 ,2.0  - this draws 1x1 square one pixel away from the corner
			// 1.51,1.51 2.0 ,2.0  - this draws nothing
			// 1.0 ,1.0  1.51,1.51 - this draws 1x1 square one pixel away from the corner
			// 1.5 ,1.5  1.51,1.51 - this draws 1x1 square one pixel away from the corner

			// 0.51,0.51 2.50,2.50 - this draws 1x1 square one pixel away from the corner

			//   /
			// |  .  |  .  |  .  |  .  |  .  |

			// rect : pixel coordinates start from pixel corner, not from pixel center.
			//        (0.5 - 1.5] - rendering starts from second pixel. smaller-equal than 0.5 is first pixel.
			//ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0.51, 0.51), ImVec2(2.50, 2.50), ImColor(128,128,128,255));

			ImGui::GetWindowDrawList()->AddRectFilled(
				ImVec2(up_left_x, up_left_y),
				ImVec2(down_right_x, down_right_y),
				ImColor(128, 128, 128, 255));

			// text : pixel coordinates start from pixel corner, not from pixel center.
			// [0., 1.) - render starts inside first pixel. 1. is start of the second pixel.
			//ImGui::GetWindowDrawList()->AddText(ImGui::GetWindowFont(), ImGui::GetWindowFont()->FontSize, ImVec2(1., 1.), ImColor(255,255,255,255), txt, NULL, 0, NULL);

			ImGui::GetWindowDrawList()->AddText(
				ImGui::GetWindowFont(),
				ImGui::GetWindowFont()->FontSize,
				ImVec2(x, y),
				ImColor(255, 255, 255, 255),
				txt, NULL, 0, NULL);

			// textile saab floor teha ok.
			// aga rectile? round(x-0.5)
			// tekstil on pixel 1, x coord [1..2)

			ImGui::End();
		}

		//static bool show_test_window;
		//ImGui::ShowTestWindow(&show_test_window);

		// Rendering
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
