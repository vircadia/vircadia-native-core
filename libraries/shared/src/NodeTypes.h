//
//  NodeTypes.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 2013/04/09
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Single byte/character Node Types used to identify various nodes in the system.
//  For example, an node whose is 'V' is always a voxel server.

#ifndef hifi_NodeTypes_h
#define hifi_NodeTypes_h

// NOTE: If you add a new NODE_TYPE_XXX then you also should add a new NODE_TYPE_NAME_XXX and a new "case" to the
//       switch statement in Node.cpp specifically Node::getTypeName().
//       If you don't then it will make things harder on your co-developers in debugging because the Node
//       class won't know the name and will report it as "Unknown".

typedef char NODE_TYPE;
const NODE_TYPE NODE_TYPE_DOMAIN = 'D';
const NODE_TYPE NODE_TYPE_VOXEL_SERVER = 'V';
const NODE_TYPE NODE_TYPE_AGENT = 'I';
const NODE_TYPE NODE_TYPE_AUDIO_MIXER = 'M';
const NODE_TYPE NODE_TYPE_AVATAR_MIXER = 'W';
const NODE_TYPE NODE_TYPE_AUDIO_INJECTOR = 'A';
const NODE_TYPE NODE_TYPE_ANIMATION_SERVER = 'a';
const NODE_TYPE NODE_TYPE_ASSIGNEE = 1;

#endif
