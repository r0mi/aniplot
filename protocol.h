// Elmo Trolla, 2019
// Licence: pick one - public domain / UNLICENCE (https://www.unlicense.org) / MIT (https://opensource.org/licenses/MIT).

#ifndef PROTOCOL_H
#define PROTOCOL_H

//
// aniplot network protocol
//

enum {
	P_CHANNEL_SAMPLES = 10,
	P_CHANNEL_INFO    = 11,
	P_LAYOUT          = 12,
};

#pragma pack(push,1)

/* packet version 1
struct p_channel_info {
	uint8_t  packet_type;
	uint8_t  packet_version; // 1
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
};*/

// stream position on screen. coordinates 0..1, x2 has to be > x1, same for y.
struct stream_pos_t {
	uint8_t stream_id;
	float   x1;
	float   y1;
	float   x2;
	float   y2;
};

struct p_layout {
	uint8_t packet_type;
	uint8_t packet_version; // 1
	uint8_t num_streams; // how many stream_pos_t structs follow
	stream_pos_t stream_pos[];
};

struct p_channel_info {
	uint8_t  packet_type;
	uint8_t  packet_version; // 2
	uint8_t  stream_id;

	uint8_t  channel_index; // channel index in stream. starts from 0.
	uint8_t  channel_name[51]; // zero-terminated
	uint8_t  unit[51]; // zero-terminated. only used if channel_index is 0.
	uint8_t  datatype; // "b", "f", "B", "d", "i", "u", "I", "U", "h", "H"; // only f is supported
	uint8_t  reserved;
	uint32_t line_color_rgba;
	float    line_width;
	// used to draw visual limits. if you know your signal is for example 0..5V, use 0 as min and 5 as max here.
	// these are only used if channel_index is 0.
	// TODO: can be swapped? what happens then? if portal is y-mirrored?
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

#endif // PROTOCOL_H

