//
//  RayPickManager.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPickManager_h
#define hifi_RayPickManager_h

#include "RegisteredMetaTypes.h"

#include <QHash>
#include <memory>
#include <QtCore/QObject>

class RayPick;

enum RayPickMask {
    PICK_NOTHING = 0,
    PICK_ENTITIES = 1,
    PICK_OVERLAYS = 2,
    PICK_AVATARS = 4,
    PICK_HUD = 8,

    PICK_BOUNDING_BOX = 16, // if not set, picks again physics mesh (can't pick against graphics mesh, yet)

    PICK_INCLUDE_INVISIBLE = 32, // if not set, will not intersect invisible elements, otherwise, intersects both visible and invisible elements
    PICK_INCLUDE_NONCOLLIDABLE = 64, // if not set, will not intersect noncollidable elements, otherwise, intersects both collidable and noncollidable elements

    PICK_ALL_INTERSECTIONS = 128 // if not set, returns closest intersection, otherwise, returns list of all intersections
};

// TODO:
// move/improve this and register it as a meta type
class RayPickResult {

public:
    RayPickResult() {}
    RayPickResult(const QUuid& objectID, const float distance, const glm::vec3& intersection, const glm::vec3& surfaceNormal = glm::vec3(NAN)) :
        _objectID(objectID), _distance(distance), _intersection(intersection), _surfaceNormal(surfaceNormal) {}

    const QUuid& getUID() { return _objectID; }
    const float getDistance() { return _distance; }
    const glm::vec3& getIntersection() { return _intersection; }
    const glm::vec3& getSurfaceNormal() { return _surfaceNormal; }

private:
    //Type type;
    QUuid _objectID { 0 };
    float _distance { FLT_MAX };
    glm::vec3 _intersection { NAN };
    glm::vec3 _surfaceNormal { NAN };
};

class RayPickManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned int PICK_NOTHING READ PICK_NOTHING CONSTANT)
    Q_PROPERTY(unsigned int PICK_ENTITIES READ PICK_ENTITIES CONSTANT)
    Q_PROPERTY(unsigned int PICK_OVERLAYS READ PICK_OVERLAYS CONSTANT)
    Q_PROPERTY(unsigned int PICK_AVATARS READ PICK_AVATARS CONSTANT)
    Q_PROPERTY(unsigned int PICK_HUD READ PICK_HUD CONSTANT)
    Q_PROPERTY(unsigned int PICK_BOUNDING_BOX READ PICK_BOUNDING_BOX CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_INVISIBLE READ PICK_INCLUDE_INVISIBLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_NONCOLLIDABLE READ PICK_INCLUDE_NONCOLLIDABLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_ALL_INTERSECTIONS READ PICK_ALL_INTERSECTIONS CONSTANT)

public:
    static RayPickManager& getInstance();

    void update();
    bool RayPickManager::checkAndCompareCachedResults(std::shared_ptr<RayPick> rayPick, QPair<glm::vec3, glm::vec3>& ray, QHash<QPair<glm::vec3, glm::vec3>, QHash<unsigned int, RayPickResult>> cache, RayPickResult& res, unsigned int mask);
    unsigned int addRayPick(std::shared_ptr<RayPick> rayPick);
    void removeRayPick(const unsigned int uid);
    void enableRayPick(const unsigned int uid);
    void disableRayPick(const unsigned int uid);

private:
    QHash<unsigned int, std::shared_ptr<RayPick>> _rayPicks;
    unsigned int _nextUID { 1 }; // 0 is invalid

    const unsigned int PICK_NOTHING() { return RayPickMask::PICK_NOTHING; }
    const unsigned int PICK_ENTITIES() { return RayPickMask::PICK_ENTITIES; }
    const unsigned int PICK_OVERLAYS() { return RayPickMask::PICK_OVERLAYS; }
    const unsigned int PICK_AVATARS() { return RayPickMask::PICK_AVATARS; }
    const unsigned int PICK_HUD() { return RayPickMask::PICK_HUD; }
    const unsigned int PICK_BOUNDING_BOX() { return RayPickMask::PICK_BOUNDING_BOX; }
    const unsigned int PICK_INCLUDE_INVISIBLE() { return RayPickMask::PICK_INCLUDE_INVISIBLE; }
    const unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return RayPickMask::PICK_INCLUDE_NONCOLLIDABLE; }
    const unsigned int PICK_ALL_INTERSECTIONS() { return RayPickMask::PICK_ALL_INTERSECTIONS; }

};

#endif hifi_RayPickManager_h