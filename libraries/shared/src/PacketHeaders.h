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

typedef char PACKET_TYPE;
const PACKET_TYPE PACKET_TYPE_DOMAIN = 'D';
const PACKET_TYPE PACKET_TYPE_PING = 'P';
const PACKET_TYPE PACKET_TYPE_PING_REPLY = 'R';
const PACKET_TYPE PACKET_TYPE_HEAD_DATA = 'H';
const PACKET_TYPE PACKET_TYPE_Z_COMMAND = 'Z';
const PACKET_TYPE PACKET_TYPE_INJECT_AUDIO = 'I';
const PACKET_TYPE PACKET_TYPE_MIXED_AUDIO = 'A';
const PACKET_TYPE PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO = 'M';
const PACKET_TYPE PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO = 'm';
const PACKET_TYPE PACKET_TYPE_SET_VOXEL = 'S';
const PACKET_TYPE PACKET_TYPE_SET_VOXEL_DESTRUCTIVE = 'O';
const PACKET_TYPE PACKET_TYPE_ERASE_VOXEL = 'E';
const PACKET_TYPE PACKET_TYPE_VOXEL_DATA = 'V';
const PACKET_TYPE PACKET_TYPE_VOXEL_DATA_MONOCHROME = 'v';
const PACKET_TYPE PACKET_TYPE_BULK_AVATAR_DATA = 'X';
const PACKET_TYPE PACKET_TYPE_AVATAR_VOXEL_URL = 'U';
const PACKET_TYPE PACKET_TYPE_AVATAR_FACE_VIDEO = 'F';
const PACKET_TYPE PACKET_TYPE_TRANSMITTER_DATA_V2 = 'T';
const PACKET_TYPE PACKET_TYPE_ENVIRONMENT_DATA = 'e';
const PACKET_TYPE PACKET_TYPE_DOMAIN_LIST_REQUEST = 'L';
const PACKET_TYPE PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY = 'C';

// this is a test...

typedef char PACKET_VERSION;

PACKET_VERSION versionForPacketType(PACKET_TYPE type);

bool packetVersionMatch(unsigned char* packetHeader);

int populateTypeAndVersion(unsigned char* destinationHeader, PACKET_TYPE type);
int numBytesForPacketHeader(unsigned char* packetHeader);

const int MAX_PACKET_HEADER_BYTES = sizeof(PACKET_TYPE) + sizeof(PACKET_VERSION);

// These are supported Z-Command
#define ERASE_ALL_COMMAND "erase all"
#define ADD_SCENE_COMMAND "add scene"
#define TEST_COMMAND      "a message"

#endif
