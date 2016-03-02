import socket

ADDR = ("localhost", 59100)

print("UDP target addr:", ADDR)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


import math
import time
import struct


def create_packet_from_stream(samples_list):
	# packet: packet type, stream id, datatype, stream
	# datatype "b","f","B","d","i","u","I","U","h","H"
	return struct.pack("<BBB" +"f"*len(samples_list), 10, 1, ord('f'), *samples_list)

while 1:
	time.sleep(0.001)
	y = math.sin( math.radians(time.time() * 360.*100) )
	p = create_packet_from_stream([y])
	sock.sendto(p, ADDR)
