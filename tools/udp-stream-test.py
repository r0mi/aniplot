import socket

ADDR = ("localhost", 59100)

print("UDP target addr:", ADDR)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


import math
import time
import struct


"""
#define P_CHANNEL_SAMPLES 10
#define P_CHANNEL_INFO 11
#define P_LAYOUT 12

#pragma pack(push,1)

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
	float    line_width; // rendering system will render thinner than 1 as 1 at the moment.
	// used to draw visual limits. if you know your signal is for example 0..5V, use 0 as min and 5 as max here.
	// these are only used if channel_index is 0.
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

"""


P_CHANNEL_SAMPLES = 10
P_CHANNEL_INFO    = 11
P_LAYOUT          = 12


def build_layout(stream_id, x1, y1, x2, y2):
	return struct.pack("<BBBBffff", P_LAYOUT, 1, 1, stream_id, x1, y1, x2, y2)

def build_channel_info(stream_id, channel_index, channel_name, unit, line_color_rgba8, line_width, value_limits, portal):
	return struct.pack("<BBBB51s51sBBBBBBfffffff",
		P_CHANNEL_INFO,
		2, # packet_version
		stream_id, channel_index,
		bytes(channel_name[:50], "utf-8"), bytes(unit[:50], "utf-8"),
		ord('f'), 0,
		line_color_rgba8[0], line_color_rgba8[1], line_color_rgba8[2], line_color_rgba8[3],
		line_width,
		value_limits[0], value_limits[1],
		portal[0], portal[1], portal[2], portal[3])

def build_channel_samples_packet(stream_id, channel_index, samples_list):
	return struct.pack("<BBBBBBH" +"f"*len(samples_list), P_CHANNEL_SAMPLES, 1, stream_id, channel_index, 0, ord('f'), len(samples_list)*4, *samples_list)


def send_setup_info():
	portal = (0, 0, 1000, 1)
	visual_value_limits = (-3, 3)
	sock.sendto( build_channel_info(0, 0, "x-gyro", "deg/s", (255, 100, 100, 255), 3, visual_value_limits, portal), ADDR )
	sock.sendto( build_channel_info(0, 1, "y-gyro", "deg/s", (100, 255, 100, 255), 1, visual_value_limits, portal), ADDR )
	sock.sendto( build_channel_info(1, 0, "x-acc",  "m2/s",  (100, 100, 255, 255), 2, visual_value_limits, portal), ADDR )
	sock.sendto( build_channel_info(2, 0, "z-acc",  "m2/s",  (255, 100, 255, 255), 2, visual_value_limits, portal), ADDR )
	#sock.sendto( build_channel_info(2, 0, "z-acc",  "m2/s",  (100, 100, 255, 255), 2, visual_value_limits, portal), ADDR )

	sock.sendto( build_layout(0, 0, 0, 0.5, 0.3), ADDR )
	sock.sendto( build_layout(1, 0.5, 0, 1, 0.3), ADDR )
	sock.sendto( build_layout(2, 0, 0.3, 1, 1),   ADDR )


t_last_sent_channel_infos = 0.

while 1:
	time.sleep(0.001)
	t = time.time()

	# repeat channel info and layout packets every second. this way we get pretty colors and correct units even after restarting aniplot
	if t_last_sent_channel_infos < t - 1.:
		send_setup_info()
		t_last_sent_channel_infos = t

	y1 = math.sin(t) * math.sin(t*1.3) + math.sin(t*3.4)
	y2 = math.sin(t*12.) * math.sin(t*1.1) + 2.*math.sin(t*11.4)
	y3 = math.sin(t*3.1) * math.sin(t*7.3) + math.sin(t*9.4)
	y4 = math.sin(t*8.1) * math.sin(t*14.3) + math.sin(t*2.4)

	#sock.sendto( build_channel_samples_packet(0, 0, [y1, y1*0.9, y1*0.8, y1*0.7]) )
	sock.sendto( build_channel_samples_packet(0, 0, [y1]), ADDR )
	sock.sendto( build_channel_samples_packet(0, 1, [y2]), ADDR )
	sock.sendto( build_channel_samples_packet(1, 0, [y3]), ADDR )
	sock.sendto( build_channel_samples_packet(2, 0, [y4]), ADDR )
