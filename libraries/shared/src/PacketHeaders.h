//
//  PacketHeaders.h
//  hifi
//
//  Created by Stephen Birarda on 4/8/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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
const PACKET_HEADER PACKET_HEADER_MIXED_AUDIO = 'A';
const PACKET_HEADER PACKET_HEADER_MICROPHONE_AUDIO_NO_ECHO = 'M';
const PACKET_HEADER PACKET_HEADER_MICROPHONE_AUDIO_WITH_ECHO = 'm';
const PACKET_HEADER PACKET_HEADER_SET_VOXEL = 'S';
const PACKET_HEADER PACKET_HEADER_SET_VOXEL_DESTRUCTIVE = 'O';
const PACKET_HEADER PACKET_HEADER_ERASE_VOXEL = 'E';
const PACKET_HEADER PACKET_HEADER_VOXEL_DATA = 'V';
const PACKET_HEADER PACKET_HEADER_VOXEL_DATA_MONOCHROME = 'v';
const PACKET_HEADER PACKET_HEADER_BULK_AVATAR_DATA = 'X';
const PACKET_HEADER PACKET_HEADER_AVATAR_VOXEL_URL = 'U';
const PACKET_HEADER PACKET_HEADER_TRANSMITTER_DATA_V2 = 'T';
const PACKET_HEADER PACKET_HEADER_ENVIRONMENT_DATA = 'e';
const PACKET_HEADER PACKET_HEADER_DOMAIN_LIST_REQUEST = 'L';
const PACKET_HEADER PACKET_HEADER_DOMAIN_REPORT_FOR_DUTY = 'C';

typedef char PACKET_VERSION;
PACKET_VERSION packetVersion(PACKET_HEADER header);

// These are supported Z-Command
#define ERASE_ALL_COMMAND "erase all"
#define ADD_SCENE_COMMAND "add scene"
#define TEST_COMMAND      "a message"

#endif
