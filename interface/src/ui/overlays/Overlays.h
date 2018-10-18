//
//  Overlays.h
//  interface/src/ui/overlays
//
//  Modified by Zander Otavka on 7/15/15
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Exposes methods to scripts for managing `Overlay`s and `OverlayPanel`s.
//
//  YOU SHOULD NOT USE `Overlays` DIRECTLY, unless you like pain and deprecation.  Instead, use
//  the object oriented API replacement found in `examples/libraries/overlayManager.js`.  See
//  that file for docs and usage.
//

#ifndef hifi_Overlays_h
#define hifi_Overlays_h

#include <QMouseEvent>
#include <QReadWriteLock>
#include <QScriptValue>

#include <PointerEvent.h>

#include "Overlay.h"

#include "PanelAttachable.h"

class PickRay;

class OverlayPropertyResult {
public:
    OverlayPropertyResult();
    QVariant value;
};

Q_DECLARE_METATYPE(OverlayPropertyResult);

QScriptValue OverlayPropertyResultToScriptValue(QScriptEngine* engine, const OverlayPropertyResult& value);
void OverlayPropertyResultFromScriptValue(const QScriptValue& object, OverlayPropertyResult& value);

const OverlayID UNKNOWN_OVERLAY_ID = OverlayID();

/**jsdoc
 * The result of a {@link PickRay} search using {@link Overlays.findRayIntersection|findRayIntersection}.
 * @typedef {object} Overlays.RayToOverlayIntersectionResult
 * @property {boolean} intersects - <code>true</code> if the {@link PickRay} intersected with a 3D overlay, otherwise
 *     <code>false</code>.
 * @property {Uuid} overlayID - The UUID of the overlay that was intersected.
 * @property {number} distance - The distance from the {@link PickRay} origin to the intersection point.
 * @property {Vec3} surfaceNormal - The normal of the overlay surface at the intersection point.
 * @property {Vec3} intersection - The position of the intersection point.
 * @property {object} extraInfo Additional intersection details, if available.
 */
class RayToOverlayIntersectionResult {
public:
    bool intersects { false };
    OverlayID overlayID { UNKNOWN_OVERLAY_ID };
    float distance { 0.0f };
    BoxFace face { UNKNOWN_FACE };
    glm::vec3 surfaceNormal;
    glm::vec3 intersection;
    QVariantMap extraInfo;
};
Q_DECLARE_METATYPE(RayToOverlayIntersectionResult);
QScriptValue RayToOverlayIntersectionResultToScriptValue(QScriptEngine* engine, const RayToOverlayIntersectionResult& value);
void RayToOverlayIntersectionResultFromScriptValue(const QScriptValue& object, RayToOverlayIntersectionResult& value);

class ParabolaToOverlayIntersectionResult {
public:
    bool intersects { false };
    OverlayID overlayID { UNKNOWN_OVERLAY_ID };
    float distance { 0.0f };
    float parabolicDistance { 0.0f };
    BoxFace face { UNKNOWN_FACE };
    glm::vec3 surfaceNormal;
    glm::vec3 intersection;
    QVariantMap extraInfo;
};

/**jsdoc
 * The Overlays API provides facilities to create and interact with overlays. Overlays are 2D and 3D objects visible only to
 * yourself and that aren't persisted to the domain. They are used for UI.
 * @namespace Overlays
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {Uuid} keyboardFocusOverlay - Get or set the {@link Overlays.OverlayType|web3d} overlay that has keyboard focus.
 *     If no overlay has keyboard focus, get returns <code>null</code>; set to <code>null</code> or {@link Uuid|Uuid.NULL} to 
 *     clear keyboard focus.
 */

class Overlays : public QObject {
    Q_OBJECT

    Q_PROPERTY(OverlayID keyboardFocusOverlay READ getKeyboardFocusOverlay WRITE setKeyboardFocusOverlay)

public:
    Overlays();

    void init();
    void update(float deltatime);
    void renderHUD(RenderArgs* renderArgs);
    void disable();
    void enable();

    Overlay::Pointer getOverlay(OverlayID id) const;

    /// adds an overlay that's already been created
    OverlayID addOverlay(Overlay* overlay) { return addOverlay(Overlay::Pointer(overlay)); }
    OverlayID addOverlay(const Overlay::Pointer& overlay);

    RayToOverlayIntersectionResult findRayIntersectionVector(const PickRay& ray, bool precisionPicking,
        const QVector<OverlayID>& overlaysToInclude,
        const QVector<OverlayID>& overlaysToDiscard,
        bool visibleOnly = false, bool collidableOnly = false);

    ParabolaToOverlayIntersectionResult findParabolaIntersectionVector(const PickParabola& parabola, bool precisionPicking,
        const QVector<OverlayID>& overlaysToInclude,
        const QVector<OverlayID>& overlaysToDiscard,
        bool visibleOnly = false, bool collidableOnly = false);

    bool mousePressEvent(QMouseEvent* event);
    bool mouseDoublePressEvent(QMouseEvent* event);
    bool mouseReleaseEvent(QMouseEvent* event);
    bool mouseMoveEvent(QMouseEvent* event);

    void cleanupAllOverlays();

public slots:
    /**jsdoc
     * Add an overlay to the scene.
     * @function Overlays.addOverlay
     * @param {Overlays.OverlayType} type - The type of the overlay to add.
     * @param {Overlays.OverlayProperties} properties - The properties of the overlay to add.
     * @returns {Uuid} The ID of the newly created overlay if successful, otherwise {@link Uuid|Uuid.NULL}.
     * @example <caption>Add a cube overlay in front of your avatar.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     */
    OverlayID addOverlay(const QString& type, const QVariant& properties);

    /**jsdoc
     * Create a clone of an existing overlay.
     * @function Overlays.cloneOverlay
     * @param {Uuid} overlayID - The ID of the overlay to clone.
     * @returns {Uuid} The ID of the new overlay if successful, otherwise {@link Uuid|Uuid.NULL}.
     * @example <caption>Add an overlay in front of your avatar, clone it, and move the clone to be above the 
     *     original.</caption>
     * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 }));
     * var original = Overlays.addOverlay("cube", {
     *     position: position,
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * var clone = Overlays.cloneOverlay(original);
     * Overlays.editOverlay(clone, {
     *     position: Vec3.sum({ x: 0, y: 0.5, z: 0}, position)
     * });
     */
    OverlayID cloneOverlay(OverlayID id);

    /**jsdoc
     * Edit an overlay's properties.
     * @function Overlays.editOverlay
     * @param {Uuid} overlayID - The ID of the overlay to edit.
     * @param {Overlays.OverlayProperties} properties - The properties changes to make.
     * @returns {boolean} <code>true</code> if the overlay was found and edited, otherwise <code>false</code>.
     * @example <caption>Add an overlay in front of your avatar then change its color.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * var success = Overlays.editOverlay(overlay, {
     *     color: { red: 255, green: 0, blue: 0 }
     * });
     * print("Success: " + success);
     */
    bool editOverlay(OverlayID id, const QVariant& properties);

    /**jsdoc
     * Edit multiple overlays' properties.
     * @function Overlays.editOverlays
     * @param propertiesById {object.<Uuid, Overlays.OverlayProperties>} - An object with overlay IDs as keys and
     *     {@link Overlays.OverlayProperties|OverlayProperties} edits to make as values.
     * @returns {boolean} <code>true</code> if all overlays were found and edited, otherwise <code>false</code> (some may have
     *     been found and edited).
     * @example <caption>Create two overlays in front of your avatar then change their colors.</caption>
     * var overlayA = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var overlayB = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * var overlayEdits = {};
     * overlayEdits[overlayA] = { color: { red: 255, green: 0, blue: 0 } };
     * overlayEdits[overlayB] = { color: { red: 0, green: 255, blue: 0 } };
     * var success = Overlays.editOverlays(overlayEdits);
     * print("Success: " + success);
     */
    bool editOverlays(const QVariant& propertiesById);

    /**jsdoc
     * Delete an overlay.
     * @function Overlays.deleteOverlay
     * @param {Uuid} overlayID - The ID of the overlay to delete.
     * @example <caption>Create an overlay in front of your avatar then delete it.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay: " + overlay);
     * Overlays.deleteOverlay(overlay);
     */
    void deleteOverlay(OverlayID id);

    /**jsdoc
     * Get the type of an overlay.
     * @function Overlays.getOverlayType
     * @param {Uuid} overlayID - The ID of the overlay to get the type of.
     * @returns {Overlays.OverlayType} The type of the overlay if found, otherwise an empty string.
     * @example <caption>Create an overlay in front of your avatar then get and report its type.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var type = Overlays.getOverlayType(overlay);
     * print("Type: " + type);
     */
    QString getOverlayType(OverlayID overlayId);

    /**jsdoc
     * Get the overlay script object. In particular, this is useful for accessing the event bridge for a <code>web3d</code> 
     * overlay.
     * @function Overlays.getOverlayObject
     * @param {Uuid} overlayID - The ID of the overlay to get the script object of.
     * @returns {object} The script object for the overlay if found.
     * @example <caption>Receive "hello" messages from a <code>web3d</code> overlay.</caption>
     * // HTML file: name "web3d.html".
     * <!DOCTYPE html>
     * <html>
     * <head>
     *     <title>HELLO</title>
     * </head>
     * <body>
     *     <h1>HELLO</h1></h1>
     *     <script>
     *         setInterval(function () {
     *             EventBridge.emitWebEvent("hello");
     *         }, 2000);
     *     </script>
     * </body>
     * </html>
     *
     * // Script file.
     * var web3dOverlay = Overlays.addOverlay("web3d", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.5, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     url: Script.resolvePath("web3d.html"),
     *     alpha: 1.0
     * });
     *
     * function onWebEventReceived(event) {
     *     print("onWebEventReceived() : " + JSON.stringify(event));
     * }
     *
     * overlayObject = Overlays.getOverlayObject(web3dOverlay);
     * overlayObject.webEventReceived.connect(onWebEventReceived);
     *
     * Script.scriptEnding.connect(function () {
     *     Overlays.deleteOverlay(web3dOverlay);
     * });
     */
    QObject* getOverlayObject(OverlayID id);

    /**jsdoc
     * Get the ID of the 2D overlay at a particular point on the screen or HUD.
     * @function Overlays.getOverlayAtPoint
     * @param {Vec2} point - The point to check for an overlay.
     * @returns {Uuid} The ID of the 2D overlay at the specified point if found, otherwise <code>null</code>.
     * @example <caption>Create a 2D overlay and add an event function that reports the overlay clicked on, if any.</caption>
     * var overlay = Overlays.addOverlay("rectangle", {
     *     bounds: { x: 100, y: 100, width: 200, height: 100 },
     *     color: { red: 255, green: 255, blue: 255 }
     * });
     * print("Created: " + overlay);
     *
     * Controller.mousePressEvent.connect(function (event) {
     *     var overlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
     *     print("Clicked: " + overlay);
     * });
     */
    OverlayID getOverlayAtPoint(const glm::vec2& point);

    /**jsdoc
     * Get the value of a 3D overlay's property.
     * @function Overlays.getProperty
     * @param {Uuid} overlayID - The ID of the overlay. <em>Must be for a 3D {@link Overlays.OverlayType|OverlayType}.</em>
     * @param {string} property - The name of the property value to get.
     * @returns {object} The value of the property if the 3D overlay and property can be found, otherwise
     *     <code>undefined</code>.
     * @example <caption>Create an overlay in front of your avatar then report its alpha property value.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var alpha = Overlays.getProperty(overlay, "alpha");
     * print("Overlay alpha: " + alpha);
     */
    OverlayPropertyResult getProperty(OverlayID id, const QString& property);

    /**jsdoc
     * Get the values of an overlay's properties.
     * @function Overlays.getProperties
     * @param {Uuid} overlayID - The ID of the overlay.
     * @param {Array.<string>} properties - An array of names of properties to get the values of.
     * @returns {Overlays.OverlayProperties} The values of valid properties if the overlay can be found, otherwise 
     *     <code>undefined</code>.
     * @example <caption>Create an overlay in front of your avatar then report some of its properties.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var properties = Overlays.getProperties(overlay, ["color", "alpha", "grabbable"]);
     * print("Overlay properties: " + JSON.stringify(properties));
     */
    OverlayPropertyResult getProperties(const OverlayID& id, const QStringList& properties);

    /**jsdoc
     * Get the values of multiple overlays' properties.
     * @function Overlays.getOverlaysProperties
     * @param propertiesById {object.<Uuid, Array.<string>>} - An object with overlay IDs as keys and arrays of the names of 
     *     properties to get for each as values.
     * @returns {object.<Uuid, Overlays.OverlayProperties>} An object with overlay IDs as keys and
     *     {@link Overlays.OverlayProperties|OverlayProperties} as values.
     * @example <caption>Create two cube overlays in front of your avatar then get some of their properties.</caption>
     * var overlayA = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var overlayB = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * var propertiesToGet = {};
     * propertiesToGet[overlayA] = ["color", "alpha"];
     * propertiesToGet[overlayB] = ["dimensions"];
     * var properties = Overlays.getOverlaysProperties(propertiesToGet);
     * print("Overlays properties: " + JSON.stringify(properties));
     */
    OverlayPropertyResult getOverlaysProperties(const QVariant& overlaysProperties);

    /**jsdoc
     *  Find the closest 3D overlay intersected by a {@link PickRay}. Overlays with their <code>drawInFront</code> property set  
     * to <code>true</code> have priority over overlays that don't, except that tablet overlays have priority over any  
     * <code>drawInFront</code> overlays behind them. I.e., if a <code>drawInFront</code> overlay is behind one that isn't  
     * <code>drawInFront</code>, the <code>drawInFront</code> overlay is returned, but if a tablet overlay is in front of a  
     * <code>drawInFront</code> overlay, the tablet overlay is returned.
     * @function Overlays.findRayIntersection
     * @param {PickRay} pickRay - The PickRay to use for finding overlays.
     * @param {boolean} [precisionPicking=false] - <em>Unused</em>; exists to match Entity API.
     * @param {Array.<Uuid>} [overlayIDsToInclude=[]] - If not empty then the search is restricted to these overlays.
     * @param {Array.<Uuid>} [overlayIDsToExclude=[]] - Overlays to ignore during the search.
     * @param {boolean} [visibleOnly=false] - <em>Unused</em>; exists to match Entity API.
     * @param {boolean} [collidableOnly=false] - <em>Unused</em>; exists to match Entity API.
     * @returns {Overlays.RayToOverlayIntersectionResult} The closest 3D overlay intersected by <code>pickRay</code>, taking
     *     into account <code>overlayIDsToInclude</code> and <code>overlayIDsToExclude</code> if they're not empty.
     * @example <caption>Create a cube overlay in front of your avatar. Report 3D overlay intersection details for mouse 
     *     clicks.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     *
     * Controller.mousePressEvent.connect(function (event) {
     *     var pickRay = Camera.computePickRay(event.x, event.y);
     *     var intersection = Overlays.findRayIntersection(pickRay);
     *     print("Intersection: " + JSON.stringify(intersection));
     * });
     */
    RayToOverlayIntersectionResult findRayIntersection(const PickRay& ray,
                                                       bool precisionPicking = false,
                                                       const QScriptValue& overlayIDsToInclude = QScriptValue(),
                                                       const QScriptValue& overlayIDsToDiscard = QScriptValue(),
                                                       bool visibleOnly = false,
                                                       bool collidableOnly = false);

    /**jsdoc
     * Return a list of 3D overlays with bounding boxes that touch a search sphere.
     * @function Overlays.findOverlays
     * @param {Vec3} center - The center of the search sphere.
     * @param {number} radius - The radius of the search sphere.
     * @returns {Uuid[]} An array of overlay IDs with bounding boxes that touch a search sphere.
     * @example <caption>Create two cube overlays in front of your avatar then search for overlays near your avatar.</caption>
     * var overlayA = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay A: " + overlayA);
     * var overlayB = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.3, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay B: " + overlayB);
     *
     * var overlaysFound = Overlays.findOverlays(MyAvatar.position, 5.0);
     * print("Overlays found: " + JSON.stringify(overlaysFound));
     */
    QVector<QUuid> findOverlays(const glm::vec3& center, float radius);

    /**jsdoc
     * Check whether an overlay's assets have been loaded. For example, for an <code>image</code> overlay the result indicates
     * whether its image has been loaded.
     * @function Overlays.isLoaded
     * @param {Uuid} overlayID - The ID of the overlay to check.
     * @returns {boolean} <code>true</code> if the overlay's assets have been loaded, otherwise <code>false</code>.
     * @example <caption>Create an image overlay and report whether its image is loaded after 1s.</caption>
     * var overlay = Overlays.addOverlay("image", {
     *     bounds: { x: 100, y: 100, width: 200, height: 200 },
     *     imageURL: "https://content.highfidelity.com/DomainContent/production/Particles/wispy-smoke.png"
     * });
     * Script.setTimeout(function () {
     *     var isLoaded = Overlays.isLoaded(overlay);
     *     print("Image loaded: " + isLoaded);
     * }, 1000);
     */
    bool isLoaded(OverlayID id);

    /**jsdoc
     * Calculates the size of the given text in the specified overlay if it is a text overlay.
     * @function Overlays.textSize
     * @param {Uuid} overlayID - The ID of the overlay to use for calculation.
     * @param {string} text - The string to calculate the size of.
     * @returns {Size} The size of the <code>text</code> if the overlay is a text overlay, otherwise
     *     <code>{ height: 0, width : 0 }</code>. If the overlay is a 2D overlay, the size is in pixels; if the overlay is a 3D
     *     overlay, the size is in meters.
     * @example <caption>Calculate the size of "hello" in a 3D text overlay.</caption>
     * var overlay = Overlays.addOverlay("text3d", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -2 })),
     *     rotation: MyAvatar.orientation,
     *     text: "hello",
     *     lineHeight: 0.2
     * });
     * var textSize = Overlays.textSize(overlay, "hello");
     * print("Size of \"hello\": " + JSON.stringify(textSize));
     */
    QSizeF textSize(OverlayID id, const QString& text);

    /**jsdoc
     * Get the width of the window or HUD.
     * @function Overlays.width
     * @returns {number} The width, in pixels, of the Interface window if in desktop mode or the HUD if in HMD mode.
     */
    float width();

    /**jsdoc
     * Get the height of the window or HUD.
     * @function Overlays.height
     * @returns {number} The height, in pixels, of the Interface window if in desktop mode or the HUD if in HMD mode.
     */
    float height();

    /**jsdoc
     * Check if there is an overlay of a given ID.
     * @function Overlays.isAddedOverlay
     * @param {Uuid} overlayID - The ID to check.
     * @returns {boolean} <code>true</code> if an overlay with the given ID exists, <code>false</code> otherwise.
     */
    bool isAddedOverlay(OverlayID id);

    /**jsdoc
     * Generate a mouse press event on an overlay.
     * @function Overlays.sendMousePressOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a mouse press event on.
     * @param {PointerEvent} event - The mouse press event details.
     * @example <caption>Create a 2D rectangle overlay plus a 3D cube overlay and generate mousePressOnOverlay events for the 2D
     * overlay.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("3D overlay: " + overlay);
     *
     * var overlay = Overlays.addOverlay("rectangle", {
     *     bounds: { x: 100, y: 100, width: 200, height: 100 },
     *     color: { red: 255, green: 255, blue: 255 }
     * });
     * print("2D overlay: " + overlay);
     *
     * // Overlays.mousePressOnOverlay by default applies only to 3D overlays.
     * Overlays.mousePressOnOverlay.connect(function(overlayID, event) {
     *     print("Clicked: " + overlayID);
     * });
     *
     * Controller.mousePressEvent.connect(function (event) {
     *     // Overlays.getOverlayAtPoint applies only to 2D overlays.
     *     var overlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
     *     if (overlay) {
     *         Overlays.sendMousePressOnOverlay(overlay, {
     *             type: "press",
     *             id: 0,
     *             pos2D: event
     *         });
     *     }
     * });
     */
    void sendMousePressOnOverlay(const OverlayID& overlayID, const PointerEvent& event);

    /**jsdoc
     * Generate a mouse release event on an overlay.
     * @function Overlays.sendMouseReleaseOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a mouse release event on.
     * @param {PointerEvent} event - The mouse release event details.
     */
    void sendMouseReleaseOnOverlay(const OverlayID& overlayID, const PointerEvent& event);

    /**jsdoc
     * Generate a mouse move event on an overlay.
     * @function Overlays.sendMouseMoveOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a mouse move event on.
     * @param {PointerEvent} event - The mouse move event details.
     */
    void sendMouseMoveOnOverlay(const OverlayID& overlayID, const PointerEvent& event);

    /**jsdoc
     * Generate a hover enter event on an overlay.
     * @function Overlays.sendHoverEnterOverlay
     * @param {Uuid} id - The ID of the overlay to generate a hover enter event on.
     * @param {PointerEvent} event - The hover enter event details.
     */
    void sendHoverEnterOverlay(const OverlayID& overlayID, const PointerEvent& event);

    /**jsdoc
     * Generate a hover over event on an overlay.
     * @function Overlays.sendHoverOverOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a hover over event on.
     * @param {PointerEvent} event - The hover over event details.
     */
    void sendHoverOverOverlay(const OverlayID& overlayID, const PointerEvent& event);

    /**jsdoc
     * Generate a hover leave event on an overlay.
     * @function Overlays.sendHoverLeaveOverlay
     * @param {Uuid} overlayID - The ID of the overlay to generate a hover leave event on.
     * @param {PointerEvent} event - The hover leave event details.
     */
    void sendHoverLeaveOverlay(const OverlayID& overlayID, const PointerEvent& event);

    /**jsdoc
     * Get the ID of the Web3D overlay that has keyboard focus.
     * @function Overlays.getKeyboardFocusOverlay
     * @returns {Uuid} The ID of the {@link Overlays.OverlayType|web3d} overlay that has focus, if any, otherwise 
     *     <code>null</code>.
     */
    OverlayID getKeyboardFocusOverlay();

    /**jsdoc
     * Set the Web3D overlay that has keyboard focus.
     * @function Overlays.setKeyboardFocusOverlay
     * @param {Uuid} overlayID - The ID of the {@link Overlays.OverlayType|web3d} overlay to set keyboard focus to. Use 
     *     <code>null</code> or {@link Uuid|Uuid.NULL} to unset keyboard focus from an overlay.
     */
    void setKeyboardFocusOverlay(const OverlayID& id);

signals:
    /**jsdoc
     * Triggered when an overlay is deleted.
     * @function Overlays.overlayDeleted
     * @param {Uuid} overlayID - The ID of the overlay that was deleted.
     * @returns {Signal}
     * @example <caption>Create an overlay then delete it after 1s.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay: " + overlay);
     *
     * Overlays.overlayDeleted.connect(function(overlayID) {
     *     print("Deleted: " + overlayID);
     * });
     * Script.setTimeout(function () {
     *     Overlays.deleteOverlay(overlay);
     * }, 1000);
     */
    void overlayDeleted(OverlayID id);

    /**jsdoc
     * Triggered when a mouse press event occurs on an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendMousePressOnOverlay|sendMousePressOnOverlay} for a 2D overlay).
     * @function Overlays.mousePressOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse press event occurred on.
     * @param {PointerEvent} event - The mouse press event details.
     * @returns {Signal}
     * @example <caption>Create a cube overlay in front of your avatar and report mouse clicks on it.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("My overlay: " + overlay);
     *
     * Overlays.mousePressOnOverlay.connect(function(overlayID, event) {
     *     if (overlayID === overlay) {
     *         print("Clicked on my overlay");
     *     }
     * });
     */
    void mousePressOnOverlay(OverlayID overlayID, const PointerEvent& event);

    /**jsdoc
     * Triggered when a mouse double press event occurs on an overlay. Only occurs for 3D overlays.
     * @function Overlays.mouseDoublePressOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse double press event occurred on.
     * @param {PointerEvent} event - The mouse double press event details.
     * @returns {Signal}
     */
    void mouseDoublePressOnOverlay(OverlayID overlayID, const PointerEvent& event);

    /**jsdoc
     * Triggered when a mouse release event occurs on an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendMouseReleaseOnOverlay|sendMouseReleaseOnOverlay} for a 2D overlay).
     * @function Overlays.mouseReleaseOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse release event occurred on.
     * @param {PointerEvent} event - The mouse release event details.
     * @returns {Signal}
     */
    void mouseReleaseOnOverlay(OverlayID overlayID, const PointerEvent& event);

    /**jsdoc
     * Triggered when a mouse move event occurs on an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendMouseMoveOnOverlay|sendMouseMoveOnOverlay} for a 2D overlay).
     * @function Overlays.mouseMoveOnOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse moved event occurred on.
     * @param {PointerEvent} event - The mouse move event details.
     * @returns {Signal}
     */
    void mouseMoveOnOverlay(OverlayID overlayID, const PointerEvent& event);

    /**jsdoc
     * Triggered when a mouse press event occurs on something other than a 3D overlay.
     * @function Overlays.mousePressOffOverlay
     * @returns {Signal}
     */
    void mousePressOffOverlay();

    /**jsdoc
     * Triggered when a mouse double press event occurs on something other than a 3D overlay.
     * @function Overlays.mouseDoublePressOffOverlay
     * @returns {Signal}
     */
    void mouseDoublePressOffOverlay();

    /**jsdoc
     * Triggered when a mouse cursor starts hovering over an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendHoverEnterOverlay|sendHoverEnterOverlay} for a 2D overlay).
     * @function Overlays.hoverEnterOverlay
     * @param {Uuid} overlayID - The ID of the overlay the mouse moved event occurred on.
     * @param {PointerEvent} event - The mouse move event details.
     * @returns {Signal}
     * @example <caption>Create a cube overlay in front of your avatar and report when you start hovering your mouse over
     *     it.</caption>
     * var overlay = Overlays.addOverlay("cube", {
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })),
     *     rotation: MyAvatar.orientation,
     *     dimensions: { x: 0.3, y: 0.3, z: 0.3 },
     *     solid: true
     * });
     * print("Overlay: " + overlay);
     * Overlays.hoverEnterOverlay.connect(function(overlayID, event) {
     *     print("Hover enter: " + overlayID);
     * });
     */
    void hoverEnterOverlay(OverlayID overlayID, const PointerEvent& event);

    /**jsdoc
     * Triggered when a mouse cursor continues hovering over an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendHoverOverOverlay|sendHoverOverOverlay} for a 2D overlay).
     * @function Overlays.hoverOverOverlay
     * @param {Uuid} overlayID - The ID of the overlay the hover over event occurred on.
     * @param {PointerEvent} event - The hover over event details.
     * @returns {Signal}
     */
    void hoverOverOverlay(OverlayID overlayID, const PointerEvent& event);

    /**jsdoc
     * Triggered when a mouse cursor finishes hovering over an overlay. Only occurs for 3D overlays (unless you use 
     *     {@link Overlays.sendHoverLeaveOverlay|sendHoverLeaveOverlay} for a 2D overlay).
     * @function Overlays.hoverLeaveOverlay
     * @param {Uuid} overlayID - The ID of the overlay the hover leave event occurred on.
     * @param {PointerEvent} event - The hover leave event details.
     * @returns {Signal}
     */
    void hoverLeaveOverlay(OverlayID overlayID, const PointerEvent& event);

private:
    void cleanupOverlaysToDelete();

    mutable QMutex _mutex { QMutex::Recursive };
    QMap<OverlayID, Overlay::Pointer> _overlaysHUD;
    QMap<OverlayID, Overlay::Pointer> _overlaysWorld;

    QList<Overlay::Pointer> _overlaysToDelete;
    unsigned int _stackOrder { 1 };

    bool _enabled = true;
    std::atomic<bool> _shuttingDown{ false };

    PointerEvent calculateOverlayPointerEvent(OverlayID overlayID, PickRay ray, RayToOverlayIntersectionResult rayPickResult,
        QMouseEvent* event, PointerEvent::EventType eventType);

    OverlayID _currentClickingOnOverlayID { UNKNOWN_OVERLAY_ID };
    OverlayID _currentHoverOverOverlayID { UNKNOWN_OVERLAY_ID };

private slots:
    void mousePressPointerEvent(const OverlayID& overlayID, const PointerEvent& event);
    void mouseMovePointerEvent(const OverlayID& overlayID, const PointerEvent& event);
    void mouseReleasePointerEvent(const OverlayID& overlayID, const PointerEvent& event);
    void hoverEnterPointerEvent(const OverlayID& overlayID, const PointerEvent& event);
    void hoverOverPointerEvent(const OverlayID& overlayID, const PointerEvent& event);
    void hoverLeavePointerEvent(const OverlayID& overlayID, const PointerEvent& event);
};

#endif // hifi_Overlays_h
