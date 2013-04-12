//
//  PacketHeaders.h
//  hifi
//
//  Created by Stephen Birarda on 4/8/13.
//
//
//  The packet headers below refer to the first byte of a received UDP packet transmitted between
//  any two Hifi components.  For example, a packet whose first byte is 'P' is always a ping packet.
//  

#ifndef hifi_PacketHeaders_h
#define hifi_PacketHeaders_h

const char PACKET_HEADER_DOMAIN = 'D';
const char PACKET_HEADER_PING = 'P';
const char PACKET_HEADER_PING_REPLY = 'R';
const char PACKET_HEADER_HEAD_DATA = 'H';
const char PACKET_HEADER_Z_COMMAND = 'Z';
const char PACKET_HEADER_INJECT_AUDIO = 'I';
const char PACKET_HEADER_SET_VOXEL = 'S';
const char PACKET_HEADER_ERASE_VOXEL = 'E';
const char PACKET_HEADER_VOXEL_DATA = 'V';
const char PACKET_HEADER_BULK_AVATAR_DATA = 'X';
const char PACKET_HEADER_TRANSMITTER_DATA = 't';

#endif
