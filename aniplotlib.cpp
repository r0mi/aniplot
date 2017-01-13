// Copyright (c) 2017, Elmo Trolla
// LICENSE: https://opensource.org/licenses/ISC

#define IMGUI_DEFINE_MATH_OPERATORS
#include "aniplotlib.h"

#define IMGUI_TEXTWRAP_IMPLEMENTATION
#include "imgui_textwrap.h"


#define EPSILON 0.0000001f
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))


GraphWidget::GraphWidget()
{
	draw_bottom_scrollbar = false;
	draw_top_scrollbar = false;
	anchored = true;
	m_textrend = new ImguiTextwrap();
}

GraphWidget::~GraphWidget()
{
	delete m_textrend;
}

// Does NOT take ownership.
// TODO: use uniqueptr or something here
void GraphWidget::add_graph(GraphVisual* graph_visual)
{
	graph_visuals.push_back( graph_visual );
}

// pos inside area is returned as 0..1
//ImVec2 _screenpixel_to_visualspace(ImRect area, ImVec2 pos) {}

// DoSpecialGraph - for example something that renders some buttons, special application-specific ui, .., to the same window.
//void DoGraph(const ImVec2& size) {}

// size in pixels, with scrollbars (if scrollbars are turned on)
void GraphWidget::DoGraph(const char* unique_name)
{
	if (!m_textrend->font)
		m_textrend->init(ImGui::GetWindowFont());

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;
	//if (size.y <= 0 || size.x <= 0) return;
	if (!graph_visuals.size())
		return;

	GraphVisual& graph_visual = *graph_visuals[0];
	IM_ASSERT(graph_visual.graph_channel);
	GraphChannel& graph_channel = *graph_visual.graph_channel;

	ImVec2 cursor_screenspace = ImGui::GetCursorScreenPos();
	const ImRect bb(cursor_screenspace, cursor_screenspace+ImGui::GetContentRegionAvail());

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	// TODO: test limits here. text seemed to go outside?
	draw_list->PushClipRect(ImVec4(bb.Min.x, bb.Min.y, bb.Max.x, bb.Max.y));

	const ImGuiID id = window->GetID(unique_name);

	ImGuiState& g = *GImGui;
	ImGuiIO& io = ImGui::GetIO();
	const ImGuiStyle& style = g.Style;

	//ImGui::ItemSize(bb, style.FramePadding.y); // advances cursor. not needed here.
	if (!ImGui::ItemAdd(bb, &id))
		return; // outside of screen. or also behind something?

	PortalRect screen_visualspace_portal(bb);
	ImVec2d visualmousecoord = screen_visualspace_portal.proj_vout(g.IO.MousePos); // mouse coord in graph visualspace. 0,0 is top-left corner of the graph, 1,1 bottom-right.

	//LOG_IMVEC2(visualmousecoord);

	const bool hovered = ImGui::IsHovered(bb, id);
	if (hovered) {
		ImGui::SetHoveredID(id);
	}

	const bool tab_focus_requested = ImGui::FocusableItemRegister(window, g.ActiveId == id);
	if ((tab_focus_requested || (hovered && (g.IO.MouseClicked[0] | g.IO.MouseDoubleClicked[0])))) {
		ImGui::SetActiveID(id, window);
		ImGui::FocusWindow(window);
	}

	ImGui::SetItemAllowOverlap();

	if (g.ActiveId == id) {
		if (g.IO.MouseDown[0]) {
		} else {
			ImGui::SetActiveID(0, NULL);
		}
	}

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

	bool dragging = false;

	if (g.ActiveId == id && g.IO.MouseDown[0] && !g.ActiveIdIsJustActivated) {
		dragging = true;
	}

	if (dragging) {
		// TODO: floating-point problems. different graphs might drift apart.
		//       correct way to solve it would be to make GraphVisual recursive (be able to contain a list of GraphVisual objects)
		//       and here move only the top-level GraphVisual portal.
		for (int i = 0; i < graph_visuals.size(); i++) {
			graph_visuals[i]->portal.translate( -g.IO.MouseDelta / bb.GetSize() * graph_visuals[i]->portal.size() );
		}
		ImVec2 dragdelta(ImGui::GetMouseDragDelta(0));
		if (dragdelta.x > 0)
			graph_visual.anchored = false;
	}

	float last_sample_in_visualspace = graph_visual.sample_to_visualspace(ImVec2(graph_channel.data_channel.size(), 0.)).x;
	if (last_sample_in_visualspace < 1)
		graph_visual.anchored = true;

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

void GraphWidget::_render_minmax_background(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb)
{
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
int GraphWidget::_calculate_gridlines(double min_val, double max_val, double size_pixels, double min_pixels_per_div, double* out_val_begin, double* out_val_end, double* out_val_step)
{
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

void GraphWidget::_render_grid_horlines(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb)
{
	// TODO: coordinates are NOT pixelperfect. does canvas_bb wrap pixels, or pixel edges?
	PortalRect& bb_valuespace = graph_visual.portal;

	// using "volt" as a concrete mental shortcut for the value axis. the word "value" has too many meanings.
	double volt_begin, volt_end, volt_step;
	int num_gridlines = _calculate_gridlines(bb_valuespace.min.y, bb_valuespace.max.y, canvas_bb.GetHeight(), graph_visual.grid_min_div_horizontal_pix, &volt_begin, &volt_end, &volt_step);
	if (volt_step == 0 || volt_end <= volt_begin)
		return;

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

void GraphWidget::_render_grid_horlegend(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb)
{
	PortalRect& bb_valuespace = graph_visual.portal;

	// using "volt" as a concrete mental shortcut for the value axis. the word "value" has too many meanings.
	double volt_begin, volt_end, volt_step;
	int num_gridlines = _calculate_gridlines(bb_valuespace.min.y, bb_valuespace.max.y, canvas_bb.GetHeight(), graph_visual.grid_min_div_horizontal_pix, &volt_begin, &volt_end, &volt_step);
	if (volt_step == 0 || volt_end <= volt_begin)
		return;

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

void GraphWidget::_render_grid_verlines(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb)
{
	PortalRect& bb_valuespace = graph_visual.portal;

	// using "time" as a concrete mental shortcut for the samplenum axis.
	double time_begin, time_end, time_step;
	int num_gridlines = _calculate_gridlines(bb_valuespace.min.x, bb_valuespace.max.x, canvas_bb.GetWidth(), graph_visual.grid_min_div_vertical_pix, &time_begin, &time_end, &time_step);
	if (time_step == 0 || time_end <= time_begin)
		return;

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

void GraphWidget::_render_grid_verlegend(const PortalRect& screen_value_portal, GraphVisual& graph_visual, const ImRect& canvas_bb)
{
	PortalRect& bb_valuespace = graph_visual.portal;

	// using "time" as a concrete mental shortcut for the samplenum axis.
	double time_begin, time_end, time_step;
	int num_gridlines = _calculate_gridlines(bb_valuespace.min.x, bb_valuespace.max.x, canvas_bb.GetWidth(), graph_visual.grid_min_div_vertical_pix, &time_begin, &time_end, &time_step);
	if (time_step == 0 || time_end <= time_begin)
		return;

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

void GraphWidget::_render_legend(const ImRect& canvas_bb)
{
	// canvas_bb : screenspace, window contents
	int text_h = ImGui::GetTextLineHeightWithSpacing();

	// save cursor pos so we can restore it later
	ImVec2 old_cursor_pos = ImGui::GetCursorPos();

	// TODO: how the f does this work? + 4 is for top padding, but we also have the bottom to take care of, yet everything is perfect with 4 instead of 8 here.
	int h = text_h * graph_visuals.size() + 4; // checkbox height with builtin padding
	int w = text_h * 10; // roughly how many characters wide. works only with square fonts
	int x = canvas_bb.Min.x + 43;
	int y = canvas_bb.Max.y - h - 20;

	ImGui::SetCursorScreenPos(ImVec2(x,y));

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	// screenspace coordinates
	ImVec2 p_min = ImVec2(x, y);
	ImVec2 p_max = ImVec2(x+w, y+h);

	ImColor fill_col = ImColor(0, 0, 0, 128);
	ImColor border_col = ImColor(170, 170, 170, 128);
	int rounding = 0;

	draw_list->AddRectFilled(p_min, p_max, fill_col, rounding);
	draw_list->AddRect(p_min, p_max, border_col, rounding);

	x += 4; y += 4; // pad

	ImGui::SetCursorScreenPos(ImVec2(x,y));
	ImGui::BeginGroup();

	for (int i = 0; i < graph_visuals.size(); i++) {

		if (!graph_visuals[i]->graph_channel)
			continue;

		ImGui::PushID(i+100);
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(graph_visuals[i]->line_color));

			ImGui::Checkbox(graph_visuals[i]->graph_channel->name.c_str(), &graph_visuals[i]->visible);

		ImGui::PopStyleColor(1);
		ImGui::PopID();

		// draw little horizontal line (with current graph color) inside the checkbox if checkbox is not set.
		if (!graph_visuals[i]->visible) {
			ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
			float line_y = cursor_pos.y - 11;
			float line_x = cursor_pos.x + 2;
			draw_list->AddLine(ImVec2(line_x, line_y), ImVec2(line_x + 9, line_y), ImColor(graph_visuals[i]->line_color)); // ImColor(graph_visual.ver_grid_color));
		}
	}
	ImGui::EndGroup();

	// restore cursor pos
	ImGui::SetCursorPos(old_cursor_pos);
}

void GraphWidget::_grid_timestr(double seconds, double step, char* outstr, int outstr_size)
{

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
void GraphWidget::_draw_graphlines(const PortalRect& screen_sample_portal, GraphVisual& graph_visual, const ImRect& canvas_bb)
{
	IM_ASSERT(graph_visual.graph_channel);
	GraphChannel& graph_channel = *graph_visual.graph_channel;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// get samplenums of graph window edges
	double samplespace_x1 = graph_visual.visual_to_samplespace(ImVec2d(0.,0.)).x;
	double samplespace_x2 = graph_visual.visual_to_samplespace(ImVec2d(1.,0.)).x;

	if (samplespace_x2 == samplespace_x1)
		return;

	// declare values we get from the top-level MipBuf
	double out_start_pixel, out_start_sample, out_end_pixel, out_end_sample;
	int    out_start_index, out_end_index;
	MipBuf_t<float>* mipbuf;

	graph_channel.data_channel.get_buf(
		samplespace_x1, samplespace_x2, canvas_bb.GetWidth(),
		&mipbuf,
		&out_start_pixel, &out_start_index, &out_start_sample,
		&out_end_pixel, &out_end_index, &out_end_sample);

	if (out_start_index >= out_end_index)
		return;

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
		draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), linecolor, graph_visual.line_width);
		x1 = x2; y1 = y2;

		//p1 = screen_sample_portal.proj_vout( ImVec2d(out_start_sample + dsample * c, mipbuf->get(i)->avg) );
		//draw_list->AddLine(p0.toImVec2(), p1.toImVec2(), linecolor);
		//p0 = p1;
		//c++;
	}
}

