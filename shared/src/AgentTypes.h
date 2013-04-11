//
//  AgentTypes.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 2013/04/09
//
//
//  Single byte/character Agent Types used to identify various agents in the system.
//  For example, an agent whose is 'V' is always a voxel server.
//  

#ifndef hifi_AgentTypes_h
#define hifi_AgentTypes_h

// NOTE: If you add a new AGENT_TYPE_XXX then you also should add a new AGENT_TYPE_NAME_XXX and a new "case" to the
//       switch statement in Agent.cpp specifically Agent::getTypeName().
//       If you don't then it will make things harder on your co-developers in debugging because the Agent
//       class won't know the name and will report it as "Unknown".

// Agent Type Codes
const char AGENT_TYPE_DOMAIN = 'D';
const char AGENT_TYPE_VOXEL = 'V';
const char AGENT_TYPE_INTERFACE = 'I'; // could also be injector???
const char AGENT_TYPE_AUDIO_MIXER = 'M';
const char AGENT_TYPE_AVATAR_MIXER = 'W';

#endif
