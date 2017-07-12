//
//  RayPickManager.h
//  libraries/shared/src
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPickManager_h
#define hifi_RayPickManager_h

#include <QHash>
#include <memory>

class RayPick;

enum RayPickMask {
    PICK_NOTHING = 0,
    PICK_ENTITIES = 1,
    PICK_AVATARS = 2,
    PICK_OVERLAYS = 4,
    PICK_HUD = 8,

    PICK_BOUNDING_BOX = 16, // if not set, picks again physics mesh (can't pick against graphics mesh, yet)

    PICK_INCLUDE_INVISIBLE = 32, // if not set, will not intersect invisible elements, otherwise, intersects both visible and invisible elements

    PICK_ALL_INTERSECTIONS = 64 // if not set, returns closest intersection, otherwise, returns list of all intersections
};

class RayPickManager {

public:
    static RayPickManager& getInstance();

    void update();
    unsigned int addRayPick(std::shared_ptr<RayPick> rayPick);
    void removeRayPick(const unsigned int uid);
    void enableRayPick(const unsigned int uid);
    void disableRayPick(const unsigned int uid);

private:
    QHash<unsigned int, std::shared_ptr<RayPick>> _rayPicks;
    unsigned int _nextUID { 1 }; // 0 is invalid

};

#endif hifi_RayPickManager_h