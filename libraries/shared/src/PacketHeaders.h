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
const PACKET_TYPE PACKET_TYPE_UNKNOWN = 0;
const PACKET_TYPE PACKET_TYPE_STUN_RESPONSE = 1;
const PACKET_TYPE PACKET_TYPE_DOMAIN = 'D';
const PACKET_TYPE PACKET_TYPE_PING = 'P';
const PACKET_TYPE PACKET_TYPE_PING_REPLY = 'R';
const PACKET_TYPE PACKET_TYPE_KILL_NODE = 'K';
const PACKET_TYPE PACKET_TYPE_HEAD_DATA = 'H';
const PACKET_TYPE PACKET_TYPE_INJECT_AUDIO = 'I';
const PACKET_TYPE PACKET_TYPE_MIXED_AUDIO = 'A';
const PACKET_TYPE PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO = 'M';
const PACKET_TYPE PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO = 'm';
const PACKET_TYPE PACKET_TYPE_BULK_AVATAR_DATA = 'X';
const PACKET_TYPE PACKET_TYPE_AVATAR_URLS = 'U';
const PACKET_TYPE PACKET_TYPE_AVATAR_FACE_VIDEO = 'F';
const PACKET_TYPE PACKET_TYPE_TRANSMITTER_DATA_V2 = 'T';
const PACKET_TYPE PACKET_TYPE_ENVIRONMENT_DATA = 'e';
const PACKET_TYPE PACKET_TYPE_DOMAIN_LIST_REQUEST = 'L';
const PACKET_TYPE PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY = 'C';
const PACKET_TYPE PACKET_TYPE_REQUEST_ASSIGNMENT = 'r';
const PACKET_TYPE PACKET_TYPE_CREATE_ASSIGNMENT = 's';
const PACKET_TYPE PACKET_TYPE_DEPLOY_ASSIGNMENT = 'd';
const PACKET_TYPE PACKET_TYPE_DATA_SERVER_PUT = 'p';
const PACKET_TYPE PACKET_TYPE_DATA_SERVER_GET = 'g';
const PACKET_TYPE PACKET_TYPE_DATA_SERVER_SEND = 'u';
const PACKET_TYPE PACKET_TYPE_DATA_SERVER_CONFIRM = 'c';
const PACKET_TYPE PACKET_TYPE_VOXEL_QUERY = 'q';
const PACKET_TYPE PACKET_TYPE_VOXEL_DATA = 'V';
const PACKET_TYPE PACKET_TYPE_VOXEL_SET = 'S';
const PACKET_TYPE PACKET_TYPE_VOXEL_SET_DESTRUCTIVE = 'O';
const PACKET_TYPE PACKET_TYPE_VOXEL_ERASE = 'E';
const PACKET_TYPE PACKET_TYPE_OCTREE_STATS = '#';
const PACKET_TYPE PACKET_TYPE_JURISDICTION = 'J';
const PACKET_TYPE PACKET_TYPE_JURISDICTION_REQUEST = 'j';
const PACKET_TYPE PACKET_TYPE_PARTICLE_QUERY = 'Q';
const PACKET_TYPE PACKET_TYPE_PARTICLE_DATA = 'v';
const PACKET_TYPE PACKET_TYPE_PARTICLE_ADD_OR_EDIT = 'a';
const PACKET_TYPE PACKET_TYPE_PARTICLE_ERASE = 'x';
const PACKET_TYPE PACKET_TYPE_PARTICLE_ADD_RESPONSE = 'b';

typedef char PACKET_VERSION;

PACKET_VERSION versionForPacketType(PACKET_TYPE type);

bool packetVersionMatch(unsigned char* packetHeader);

int populateTypeAndVersion(unsigned char* destinationHeader, PACKET_TYPE type);
int numBytesForPacketHeader(const unsigned char* packetHeader);

const int MAX_PACKET_HEADER_BYTES = sizeof(PACKET_TYPE) + sizeof(PACKET_VERSION);

// These are supported Z-Command
#define ERASE_ALL_COMMAND "erase all"
#define ADD_SCENE_COMMAND "add scene"
#define TEST_COMMAND      "a message"

#endif
