
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

typedef char PACKET_HEADER;
const PACKET_HEADER PACKET_HEADER_DOMAIN = 'D';
const PACKET_HEADER PACKET_HEADER_PING = 'P';
const PACKET_HEADER PACKET_HEADER_PING_REPLY = 'R';
const PACKET_HEADER PACKET_HEADER_HEAD_DATA = 'H';
const PACKET_HEADER PACKET_HEADER_Z_COMMAND = 'Z';
const PACKET_HEADER PACKET_HEADER_INJECT_AUDIO = 'I';
const PACKET_HEADER PACKET_HEADER_SET_VOXEL = 'S';
const PACKET_HEADER PACKET_HEADER_ERASE_VOXEL = 'E';
const PACKET_HEADER PACKET_HEADER_VOXEL_DATA = 'V';
const PACKET_HEADER PACKET_HEADER_BULK_AVATAR_DATA = 'X';
const PACKET_HEADER PACKET_HEADER_TRANSMITTER_DATA = 't';
const PACKET_HEADER PACKET_HEADER_ENVIRONMENT_DATA = 'e';
const PACKET_HEADER PACKET_HEADER_DOMAIN_LIST_REQUEST = 'L';
const PACKET_HEADER PACKET_HEADER_DOMAIN_RFD = 'C';

#endif
