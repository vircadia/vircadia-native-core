//
//  DebugDraw.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DebugDraw_h
#define hifi_DebugDraw_h

#include <unordered_map>
#include <tuple>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class DebugDraw {
public:
    static DebugDraw& getInstance();

    DebugDraw();
    ~DebugDraw();

    // world space maker
    void addMarker(const std::string& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color);
    void removeMarker(const std::string& key);

    // myAvatar relative marker
    void addMyAvatarMarker(const std::string& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color);
    void removeMyAvatarMarker(const std::string& key);

    using MarkerInfo = std::tuple<glm::quat, glm::vec3, glm::vec4>;
    using MarkerMap = std::unordered_map<std::string, MarkerInfo>;

    //
    // accessors used by renderer
    //

    const MarkerMap& getMarkerMap() const { return _markers; }
    const MarkerMap& getMyAvatarMarkerMap() const { return _myAvatarMarkers; }
    void updateMyAvatarPos(const glm::vec3& pos) { _myAvatarPos = pos; }
    const glm::vec3& getMyAvatarPos() const { return _myAvatarPos; }
    void updateMyAvatarRot(const glm::quat& rot) { _myAvatarRot = rot; }
    const glm::quat& getMyAvatarRot() const { return _myAvatarRot; }

protected:
    MarkerMap _markers;
    MarkerMap _myAvatarMarkers;
    glm::quat _myAvatarRot;
    glm::vec3 _myAvatarPos;
};

#endif // hifi_DebugDraw_h
