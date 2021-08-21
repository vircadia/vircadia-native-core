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

/*@jsdoc
 * The <code>DebugDraw</code> API renders debug markers and lines. These markers are only visible locally; they are not visible 
 * to other users.
 *
 * @namespace DebugDraw
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */
class DebugDraw : public QObject {
    Q_OBJECT
public:
    static DebugDraw& getInstance();

    DebugDraw();
    ~DebugDraw();

    /*@jsdoc
     * Draws a line in world space, visible for a single frame. To make the line visually persist, you need to repeatedly draw 
     * it.
     * @function DebugDraw.drawRay
     * @param {Vec3} start - The start position of the line, in world coordinates.
     * @param {Vec3} end - The end position of the line, in world coordinates.
     * @param {Vec4} color - The color of the line. Each component should be in the range <code>0.0</code> &ndash; 
     * <code>1.0</code>, with <code>x</code> = red, <code>y</code> = green, <code>z</code> = blue, and <code>w</code> = alpha.
     * @example <caption>Draw a red ray from your initial avatar position to 10m in front of it.</caption>
     * var start = MyAvatar.position;
     * var end = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -10 }));
     * var color = { x: 1.0, y: 0.0, z: 0.0, w: 1.0 };
     * 
     * Script.update.connect(function () {
     *     DebugDraw.drawRay(start, end, color);
     * });
     */
    Q_INVOKABLE void drawRay(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    
    /*@jsdoc
     * Draws lines in world space, visible for a single frame. To make the lines visually persist, you need to repeatedly draw 
     * them.
     * <p><strong>Note:</strong> Currently doesn't work.
     * @function DebugDraw.drawRays
     * @param {Vec3Pair[]} lines - The start and end points of the lines to draw.
     * @param {Vec4} color - The color of the lines. Each component should be in the range <code>0.0</code> &ndash; 
     * <code>1.0</code>, with <code>x</code> = red, <code>y</code> = green, <code>z</code> = blue, and <code>w</code> = alpha.
     * @param {Vec3} [translation=0,0,0] - A translation applied to each line.
     * @param {Quat} [rotation=Quat.IDENTITY] - A rotation applied to each line.
     * @example <caption>Draw a red "V" in front of your initial avatar position.</caption>
     * var lines = [
     *     [{ x: -1, y: 0.5, z: 0 }, { x: 0, y: 0, z: 0 }],
     *     [{ x: 0, y: 0, z: 0 }, { x: 1, y: 0.5, z: 0 }]
     * ];
     * var color = { x: 1, y: 0, z: 0, w: 1 };
     * var translation = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 }));
     * var rotation = MyAvatar.orientation;
     * 
     * Script.update.connect(function () {
     *     DebugDraw.drawRays(lines, color, translation, rotation);
     * });
     */
    Q_INVOKABLE void drawRays(const std::vector<std::pair<glm::vec3, glm::vec3>>& lines, const glm::vec4& color,
                              const glm::vec3& translation = glm::vec3(0.0f, 0.0f, 0.0f), const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    /*@jsdoc
     * Adds or updates a debug marker in world coordinates. This marker is drawn every frame until it is removed using  
     * {@link DebugDraw.removeMarker|removeMarker}. If a world coordinates debug marker of the specified <code>name</code> 
     * already exists, its parameters are updated.
     * @function DebugDraw.addMarker
     * @param {string} key - A name that uniquely identifies the marker.
     * @param {Quat} rotation - The orientation of the marker in world coordinates.
     * @param {Vec3} position - The position of the market in world coordinates.
     * @param {Vec4} color - The color of the marker.
     * @param {float} size - A float between 0.0 and 1.0 (10 cm) to control the size of the marker.
     * @example <caption>Briefly draw a debug marker in front of your avatar, in world coordinates.</caption>
     * var MARKER_NAME = "my marker";
     * DebugDraw.addMarker(
     *     MARKER_NAME,
     *     Quat.ZERO,
     *     Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5})),
     *     { red: 255, green: 0, blue: 0 },
     *     1.0
     * );
     * Script.setTimeout(function () {
     *     DebugDraw.removeMarker(MARKER_NAME);
     * }, 5000);
     */
    Q_INVOKABLE void addMarker(const QString& key, const glm::quat& rotation, const glm::vec3& position,
                               const glm::vec4& color, float size = 1.0f);

    /*@jsdoc
     * Removes a debug marker that was added in world coordinates.
     * @function DebugDraw.removeMarker
     * @param {string} key - The name of the world coordinates debug marker to remove.
     */
    Q_INVOKABLE void removeMarker(const QString& key);

    /*@jsdoc
     * Adds or updates a debug marker to the world in avatar coordinates. This marker is drawn every frame until it is removed 
     * using {@link DebugDraw.removeMyAvatarMarker|removeMyAvatarMarker}. If an avatar coordinates debug marker of the 
     * specified <code>name</code> already exists, its parameters are updated. The debug marker moves with your avatar.
     * @function DebugDraw.addMyAvatarMarker
     * @param {string} key - A name that uniquely identifies the marker.
     * @param {Quat} rotation - The orientation of the marker in avatar coordinates.
     * @param {Vec3} position - The position of the market in avatar coordinates.
     * @param {Vec4} color - color of the marker.
     * @param {float} size - A float between 0.0 and 1.0 (10 cm) to control the size of the marker.
     * @example <caption>Briefly draw a debug marker in front of your avatar, in avatar coordinates.</caption>
     * var MARKER_NAME = "My avatar marker";
     * DebugDraw.addMyAvatarMarker(
     *     MARKER_NAME,
     *     Quat.ZERO,
     *     { x: 0, y: 0, z: -5 },
     *     { red: 255, green: 0, blue: 0 },
     *     1.0
     * );
     * Script.setTimeout(function () {
     *     DebugDraw.removeMyAvatarMarker(MARKER_NAME);
     * }, 5000);
     */
    Q_INVOKABLE void addMyAvatarMarker(const QString& key, const glm::quat& rotation, const glm::vec3& position,
                                       const glm::vec4& color, float size = 1.0f);

    /*@jsdoc
     * Removes a debug marker that was added in avatar coordinates.
     * @function DebugDraw.removeMyAvatarMarker
     * @param {string} key - The name of the avatar coordinates debug marker to remove.
     */
    Q_INVOKABLE void removeMyAvatarMarker(const QString& key);

    using MarkerInfo = std::tuple<glm::quat, glm::vec3, glm::vec4, float>;
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
