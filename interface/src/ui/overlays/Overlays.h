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
#include "OverlayPanel.h"

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
 * @typedef Overlays.RayToOverlayIntersectionResult
 * @property {bool} intersects True if the PickRay intersected with a 3D overlay.
 * @property {Overlays.OverlayID} overlayID The ID of the overlay that was intersected with.
 * @property {float} distance The distance from the PickRay origin to the intersection point.
 * @property {Vec3} surfaceNormal The normal of the surface that was intersected with.
 * @property {Vec3} intersection The point at which the PickRay intersected with the overlay.
 */
class RayToOverlayIntersectionResult {
public:
    bool intersects { false };
    OverlayID overlayID { UNKNOWN_OVERLAY_ID };
    float distance { 0 };
    BoxFace face;
    glm::vec3 surfaceNormal;
    glm::vec3 intersection;
    QString extraInfo;
};


Q_DECLARE_METATYPE(RayToOverlayIntersectionResult);

QScriptValue RayToOverlayIntersectionResultToScriptValue(QScriptEngine* engine, const RayToOverlayIntersectionResult& value);
void RayToOverlayIntersectionResultFromScriptValue(const QScriptValue& object, RayToOverlayIntersectionResult& value);

/**jsdoc
 * @typedef {int} Overlays.OverlayID
 */

/**jsdoc
 *
 * Overlays namespace...
 * @namespace Overlays
 */

class Overlays : public QObject {
    Q_OBJECT

    Q_PROPERTY(OverlayID keyboardFocusOverlay READ getKeyboardFocusOverlay WRITE setKeyboardFocusOverlay)

public:
    Overlays() {};

    void init();
    void update(float deltatime);
    void renderHUD(RenderArgs* renderArgs);
    void render3DHUDOverlays(RenderArgs* renderArgs);
    void disable();
    void enable();

    Overlay::Pointer getOverlay(OverlayID id) const;
#if OVERLAY_PANELS
    OverlayPanel::Pointer getPanel(OverlayID id) const { return _panels[id]; }
#endif

    /// adds an overlay that's already been created
    OverlayID addOverlay(Overlay* overlay) { return addOverlay(Overlay::Pointer(overlay)); }
    OverlayID addOverlay(const Overlay::Pointer& overlay);

    void setOverlayDrawHUDLayer(const OverlayID& id, const bool drawHUDLayer);

    bool mousePressEvent(QMouseEvent* event);
    bool mouseDoublePressEvent(QMouseEvent* event);
    bool mouseReleaseEvent(QMouseEvent* event);
    bool mouseMoveEvent(QMouseEvent* event);

    void cleanupAllOverlays();

public slots:
    /**jsdoc
     * Add an overlays to the scene. The properties specified will depend
     * on the type of overlay that is being created.
     *
     * @function Overlays.addOverlay
     * @param {string} type The type of the overlay to add.
     * @param {Overlays.OverlayProperties} The properties of the overlay that you want to add.
     * @return {Overlays.OverlayID} The ID of the newly created overlay.
     */
    OverlayID addOverlay(const QString& type, const QVariant& properties);

    /**jsdoc
     * Create a clone of an existing overlay.
     *
     * @function Overlays.cloneOverlay
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to clone.
     * @return {Overlays.OverlayID} The ID of the new overlay.
     */
    OverlayID cloneOverlay(OverlayID id);

    /**jsdoc
     * Edit an overlay's properties.
     *
     * @function Overlays.editOverlay
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to edit.
     * @return {bool} `true` if the overlay was found and edited, otherwise false.
     */
    bool editOverlay(OverlayID id, const QVariant& properties);

    /// edits an overlay updating only the included properties, will return the identified OverlayID in case of
    /// successful edit, if the input id is for an unknown overlay this function will have no effect
    bool editOverlays(const QVariant& propertiesById);

    /**jsdoc
     * Delete an overlay.
     *
     * @function Overlays.deleteOverlay
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to delete.
     */
    void deleteOverlay(OverlayID id);

    /**jsdoc
     * Get the type of an overlay.
     *
     * @function Overlays.getOverlayType
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to get the type of.
     * @return {string} The type of the overlay if found, otherwise the empty string.
     */
    QString getOverlayType(OverlayID overlayId);

    /**jsdoc
    * Get the overlay Script object.
    *
    * @function Overlays.getOverlayObject
    * @param {Overlays.OverlayID} overlayID The ID of the overlay to get the script object of.
    * @return {Object} The script object for the overlay if found.
    */
    QObject* getOverlayObject(OverlayID id);

    /**jsdoc
     * Get the ID of the overlay at a particular point on the HUD/screen.
     *
     * @function Overlays.getOverlayAtPoint
     * @param {Vec2} point The point to check for an overlay.
     * @return {Overlays.OverlayID} The ID of the overlay at the point specified.
     *     If no overlay is found, `0` will be returned.
     */
    OverlayID getOverlayAtPoint(const glm::vec2& point);

    /**jsdoc
     * Get the value of a an overlay's property.
     *
     * @function Overlays.getProperty
     * @param {Overlays.OverlayID} The ID of the overlay to get the property of.
     * @param {string} The name of the property to get the value of.
     * @return {Object} The value of the property. If the overlay or the property could
     *     not be found, `undefined` will be returned.
     */
    OverlayPropertyResult getProperty(OverlayID id, const QString& property);

    OverlayPropertyResult getProperties(const OverlayID& id, const QStringList& properties);

    OverlayPropertyResult getOverlaysProperties(const QVariant& overlaysProperties);

    /*jsdoc
     * Find the closest 3D overlay hit by a pick ray.
     *
     * @function Overlays.findRayIntersection
     * @param {PickRay} The PickRay to use for finding overlays.
     * @param {bool} Unused; Exists to match Entity interface
     * @param {List of Overlays.OverlayID} Whitelist for intersection test.
     * @param {List of Overlays.OverlayID} Blacklist for intersection test.
     * @param {bool} Unused; Exists to match Entity interface
     * @param {bool} Unused; Exists to match Entity interface
     * @return {Overlays.RayToOverlayIntersectionResult} The result of the ray cast.
     */
    RayToOverlayIntersectionResult findRayIntersection(const PickRay& ray,
                                                       bool precisionPicking = false,
                                                       const QScriptValue& overlayIDsToInclude = QScriptValue(),
                                                       const QScriptValue& overlayIDsToDiscard = QScriptValue(),
                                                       bool visibleOnly = false,
                                                       bool collidableOnly = false);

    // Same as above but with QVectors
    RayToOverlayIntersectionResult findRayIntersectionVector(const PickRay& ray, bool precisionPicking,
                                                             const QVector<OverlayID>& overlaysToInclude,
                                                             const QVector<OverlayID>& overlaysToDiscard,
                                                             bool visibleOnly = false, bool collidableOnly = false);

    /**jsdoc
     * Return a list of 3d overlays with bounding boxes that touch the given sphere
     *
     * @function Overlays.findOverlays
     * @param {Vec3} center the point to search from.
     * @param {float} radius search radius
     * @return {Overlays.OverlayID[]} list of overlays withing the radius
     */
    QVector<QUuid> findOverlays(const glm::vec3& center, float radius);

    /**jsdoc
     * Check whether an overlay's assets have been loaded. For example, if the
     * overlay is an "image" overlay, this will indicate whether the its image
     * has loaded.
     * @function Overlays.isLoaded
     * @param {Overlays.OverlayID} The ID of the overlay to check.
     * @return {bool} `true` if the overlay's assets have been loaded, otherwise `false`.
     */
    bool isLoaded(OverlayID id);

    /**jsdoc
     * Calculates the size of the given text in the specified overlay if it is a text overlay.
     * If it is a 2D text overlay, the size will be in pixels.
     * If it is a 3D text overlay, the size will be in meters.
     *
     * @function Overlays.textSize
     * @param {Overlays.OverlayID} The ID of the overlay to measure.
     * @param {string} The string to measure.
     * @return {Vec2} The size of the text.
     */
    QSizeF textSize(OverlayID id, const QString& text);

    /**jsdoc
     * Get the width of the virtual 2D HUD.
     *
     * @function Overlays.width
     * @return {float} The width of the 2D HUD.
     */
    float width();

    /**jsdoc
     * Get the height of the virtual 2D HUD.
     *
     * @function Overlays.height
     * @return {float} The height of the 2D HUD.
     */
    float height();

    /// return true if there is an overlay with that id else false
    bool isAddedOverlay(OverlayID id);

#if OVERLAY_PANELS
    OverlayID getParentPanel(OverlayID childId) const;
    void setParentPanel(OverlayID childId, OverlayID panelId);

    /// adds a panel that has already been created
    OverlayID addPanel(OverlayPanel::Pointer panel);

    /// creates and adds a panel based on a set of properties
    OverlayID addPanel(const QVariant& properties);

    /// edit the properties of a panel
    void editPanel(OverlayID panelId, const QVariant& properties);

    /// get a property of a panel
    OverlayPropertyResult getPanelProperty(OverlayID panelId, const QString& property);

    /// deletes a panel and all child overlays
    void deletePanel(OverlayID panelId);

    /// return true if there is a panel with that id else false
    bool isAddedPanel(OverlayID id) { return _panels.contains(id); }

#endif

    void sendMousePressOnOverlay(const OverlayID& overlayID, const PointerEvent& event);
    void sendMouseReleaseOnOverlay(const OverlayID& overlayID, const PointerEvent& event);
    void sendMouseMoveOnOverlay(const OverlayID& overlayID, const PointerEvent& event);

    void sendHoverEnterOverlay(const OverlayID& id, const PointerEvent& event);
    void sendHoverOverOverlay(const OverlayID& id, const PointerEvent& event);
    void sendHoverLeaveOverlay(const OverlayID& id, const PointerEvent& event);

    OverlayID getKeyboardFocusOverlay();
    void setKeyboardFocusOverlay(OverlayID id);

signals:
    /**jsdoc
     * Emitted when an overlay is deleted
     *
     * @function Overlays.overlayDeleted
     * @param {OverlayID} The ID of the overlay that was deleted.
     */
    void overlayDeleted(OverlayID id);
    void panelDeleted(OverlayID id);

    void mousePressOnOverlay(OverlayID overlayID, const PointerEvent& event);
    void mouseDoublePressOnOverlay(OverlayID overlayID, const PointerEvent& event);
    void mouseReleaseOnOverlay(OverlayID overlayID, const PointerEvent& event);
    void mouseMoveOnOverlay(OverlayID overlayID, const PointerEvent& event);
    void mousePressOffOverlay();
    void mouseDoublePressOffOverlay();

    void hoverEnterOverlay(OverlayID overlayID, const PointerEvent& event);
    void hoverOverOverlay(OverlayID overlayID, const PointerEvent& event);
    void hoverLeaveOverlay(OverlayID overlayID, const PointerEvent& event);

private:
    void cleanupOverlaysToDelete();

    mutable QMutex _mutex { QMutex::Recursive };
    QMap<OverlayID, Overlay::Pointer> _overlaysHUD;
    QMap<OverlayID, Overlay::Pointer> _overlays3DHUD;
    QMap<OverlayID, Overlay::Pointer> _overlaysWorld;

    render::ShapePlumberPointer _shapePlumber;

#if OVERLAY_PANELS
    QMap<OverlayID, OverlayPanel::Pointer> _panels;
#endif
    QList<Overlay::Pointer> _overlaysToDelete;
    unsigned int _stackOrder { 1 };

#if OVERLAY_PANELS
    QScriptEngine* _scriptEngine;
#endif
    bool _enabled = true;

    PointerEvent calculateOverlayPointerEvent(OverlayID overlayID, PickRay ray, RayToOverlayIntersectionResult rayPickResult,
        QMouseEvent* event, PointerEvent::EventType eventType);

    OverlayID _currentClickingOnOverlayID { UNKNOWN_OVERLAY_ID };
    OverlayID _currentHoverOverOverlayID { UNKNOWN_OVERLAY_ID };

    RayToOverlayIntersectionResult findRayIntersectionForMouseEvent(PickRay ray);
};

#endif // hifi_Overlays_h
