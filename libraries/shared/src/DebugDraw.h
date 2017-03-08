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

#include <mutex>
#include <unordered_map>
#include <tuple>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QObject>
#include <QString>

/**jsdoc
 * Helper functions to render ephemeral debug markers and lines.
 * DebugDraw markers and lines are only visible locally, they are not visible by other users.
 * @namespace DebugDraw
 */
class DebugDraw : public QObject {
    Q_OBJECT
public:
    static DebugDraw& getInstance();

    DebugDraw();
    ~DebugDraw();

    /**jsdoc
     * Draws a line in world space, but it will only be visible for a single frame.
     * @function DebugDraw.drawRay
     * @param {Vec3} start - start position of line in world space.
     * @param {Vec3} end - end position of line in world space.
     * @param {Vec4} color - color of line, each component should be in the zero to one range.  x = red, y = blue, z = green, w = alpha.
     */
    Q_INVOKABLE void drawRay(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);

    /**jsdoc
     * Adds a debug marker to the world. This marker will be drawn every frame until it is removed with DebugDraw.removeMarker.
     * This can be called repeatedly to change the position of the marker.
     * @function DebugDraw.addMarker
     * @param {string} key - name to uniquely identify this marker, later used for DebugDraw.removeMarker.
     * @param {Quat} rotation - start position of line in world space.
     * @param {Vec3} position - position of the marker in world space.
     * @param {Vec4} color - color of the marker.
     */
    Q_INVOKABLE void addMarker(const QString& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color);

    /**jsdoc
     * Removes debug marker from the world.  Once a marker is removed, it will no longer be visible.
     * @function DebugDraw.removeMarker
     * @param {string} key - name of marker to remove.
     */
    Q_INVOKABLE void removeMarker(const QString& key);

    /**jsdoc
     * Adds a debug marker to the world, this marker will be drawn every frame until it is removed with DebugDraw.removeMyAvatarMarker.
     * This can be called repeatedly to change the position of the marker.
     * @function DebugDraw.addMyAvatarMarker
     * @param {string} key - name to uniquely identify this marker, later used for DebugDraw.removeMyAvatarMarker.
     * @param {Quat} rotation - start position of line in avatar space.
     * @param {Vec3} position - position of the marker in avatar space.
     * @param {Vec4} color - color of the marker.
     */
    Q_INVOKABLE void addMyAvatarMarker(const QString& key, const glm::quat& rotation, const glm::vec3& position, const glm::vec4& color);

    /**jsdoc
     * Removes debug marker from the world.  Once a marker is removed, it will no longer be visible
     * @function DebugDraw.removeMyAvatarMarker
     * @param {string} key - name of marker to remove.
     */
    Q_INVOKABLE void removeMyAvatarMarker(const QString& key);

    using MarkerInfo = std::tuple<glm::quat, glm::vec3, glm::vec4>;
    using MarkerMap = std::map<QString, MarkerInfo>;
    using Ray = std::tuple<glm::vec3, glm::vec3, glm::vec4>;
    using Rays = std::vector<Ray>;

    //
    // accessors used by renderer
    //

    MarkerMap getMarkerMap() const;
    MarkerMap getMyAvatarMarkerMap() const;
    void updateMyAvatarPos(const glm::vec3& pos) { _myAvatarPos = pos; }
    const glm::vec3& getMyAvatarPos() const { return _myAvatarPos; }
    void updateMyAvatarRot(const glm::quat& rot) { _myAvatarRot = rot; }
    const glm::quat& getMyAvatarRot() const { return _myAvatarRot; }
    Rays getRays() const;
    void clearRays();

protected:
    mutable std::mutex _mapMutex;
    MarkerMap _markers;
    MarkerMap _myAvatarMarkers;
    glm::quat _myAvatarRot;
    glm::vec3 _myAvatarPos;
    Rays _rays;
};

#endif // hifi_DebugDraw_h
