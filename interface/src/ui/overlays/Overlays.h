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
#include "OverlayPanel.h"
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
    RayToOverlayIntersectionResult();
    bool intersects;
    int overlayID;
    float distance;
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

const unsigned int UNKNOWN_OVERLAY_ID = 0;

class Overlays : public QObject {
    Q_OBJECT

public:
    Overlays();

    void init();
    void update(float deltatime);
    void renderHUD(RenderArgs* renderArgs);
    void disable();
    void enable();

    Overlay::Pointer getOverlay(unsigned int id) const;
    OverlayPanel::Pointer getPanel(unsigned int id) const { return _panels[id]; }

    /// adds an overlay that's already been created
    unsigned int addOverlay(Overlay* overlay) { return addOverlay(Overlay::Pointer(overlay)); }
    unsigned int addOverlay(Overlay::Pointer overlay);

    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

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
    unsigned int addOverlay(const QString& type, const QVariant& properties);

    /**jsdoc
     * Create a clone of an existing overlay.
     *
     * @function Overlays.cloneOverlay
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to clone.
     * @return {Overlays.OverlayID} The ID of the new overlay.
     */
    unsigned int cloneOverlay(unsigned int id);

    /**jsdoc
     * Edit an overlay's properties. 
     *
     * @function Overlays.editOverlay
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to edit.
     * @return {bool} `true` if the overlay was found and edited, otherwise false.
     */
    bool editOverlay(unsigned int id, const QVariant& properties);

    /// edits an overlay updating only the included properties, will return the identified OverlayID in case of
    /// successful edit, if the input id is for an unknown overlay this function will have no effect
    bool editOverlays(const QVariant& propertiesById);

    /**jsdoc
     * Delete an overlay.
     *
     * @function Overlays.deleteOverlay
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to delete.
     */
    void deleteOverlay(unsigned int id);

    /**jsdoc
     * Get the type of an overlay.
     *
     * @function Overlays.getOverlayType
     * @param {Overlays.OverlayID} overlayID The ID of the overlay to get the type of.
     * @return {string} The type of the overlay if found, otherwise the empty string.
     */
    QString getOverlayType(unsigned int overlayId) const;

    /**jsdoc
     * Get the ID of the overlay at a particular point on the HUD/screen.
     *
     * @function Overlays.getOverlayAtPoint
     * @param {Vec2} point The point to check for an overlay.
     * @return {Overlays.OverlayID} The ID of the overlay at the point specified.
     *     If no overlay is found, `0` will be returned.
     */
    unsigned int getOverlayAtPoint(const glm::vec2& point);

    /**jsdoc
     * Get the value of a an overlay's property.
     *
     * @function Overlays.getProperty
     * @param {Overlays.OverlayID} The ID of the overlay to get the property of.
     * @param {string} The name of the property to get the value of.
     * @return {Object} The value of the property. If the overlay or the property could
     *     not be found, `undefined` will be returned.
     */
    OverlayPropertyResult getProperty(unsigned int id, const QString& property);

    /*jsdoc
     * Find the closest 3D overlay hit by a pick ray.
     *
     * @function Overlays.findRayIntersection
     * @param {PickRay} The PickRay to use for finding overlays.
     * @return {Overlays.RayToOverlayIntersectionResult} The result of the ray cast.
     */
    RayToOverlayIntersectionResult findRayIntersection(const PickRay& ray);

    /**jsdoc
     * Check whether an overlay's assets have been loaded. For example, if the
     * overlay is an "image" overlay, this will indicate whether the its image
     * has loaded.
     * @function Overlays.isLoaded
     * @param {Overlays.OverlayID} The ID of the overlay to check.
     * @return {bool} `true` if the overlay's assets have been loaded, otherwise `false`.
     */
    bool isLoaded(unsigned int id);

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
    QSizeF textSize(unsigned int id, const QString& text) const;

    /**jsdoc
     * Get the width of the virtual 2D HUD.
     *
     * @function Overlays.width
     * @return {float} The width of the 2D HUD.
     */
    float width() const;

    /**jsdoc
     * Get the height of the virtual 2D HUD.
     *
     * @function Overlays.height
     * @return {float} The height of the 2D HUD.
     */
    float height() const;

    /// return true if there is an overlay with that id else false
    bool isAddedOverlay(unsigned int id);

    unsigned int getParentPanel(unsigned int childId) const;
    void setParentPanel(unsigned int childId, unsigned int panelId);

    /// adds a panel that has already been created
    unsigned int addPanel(OverlayPanel::Pointer panel);

    /// creates and adds a panel based on a set of properties
    unsigned int addPanel(const QVariant& properties);

    /// edit the properties of a panel
    void editPanel(unsigned int panelId, const QVariant& properties);

    /// get a property of a panel
    OverlayPropertyResult getPanelProperty(unsigned int panelId, const QString& property);

    /// deletes a panel and all child overlays
    void deletePanel(unsigned int panelId);

    /// return true if there is a panel with that id else false
    bool isAddedPanel(unsigned int id) { return _panels.contains(id); }

signals:
    /**jsdoc
     * Emitted when an overlay is deleted
     *
     * @function Overlays.overlayDeleted
     * @param {OverlayID} The ID of the overlay that was deleted.
     */
    void overlayDeleted(unsigned int id);
    void panelDeleted(unsigned int id);

    void mousePressOnOverlay(unsigned int overlayID, const PointerEvent& event);
    void mouseReleaseOnOverlay(unsigned int overlayID, const PointerEvent& event);
    void mouseMoveOnOverlay(unsigned int overlayID, const PointerEvent& event);
    void mousePressOffOverlay();

    void hoverEnterOverlay(unsigned int overlayID, const PointerEvent& event);
    void hoverOverOverlay(unsigned int overlayID, const PointerEvent& event);
    void hoverLeaveOverlay(unsigned int overlayID, const PointerEvent& event);

private:
    void cleanupOverlaysToDelete();

    QMap<unsigned int, Overlay::Pointer> _overlaysHUD;
    QMap<unsigned int, Overlay::Pointer> _overlaysWorld;
    QMap<unsigned int, OverlayPanel::Pointer> _panels;
    QList<Overlay::Pointer> _overlaysToDelete;
    unsigned int _nextOverlayID;

    QReadWriteLock _lock;
    QReadWriteLock _deleteLock;
    QScriptEngine* _scriptEngine;
    bool _enabled = true;

    PointerEvent calculatePointerEvent(Overlay::Pointer overlay, PickRay ray, RayToOverlayIntersectionResult rayPickResult,
        QMouseEvent* event, PointerEvent::EventType eventType);

    unsigned int _currentClickingOnOverlayID = UNKNOWN_OVERLAY_ID;
    unsigned int _currentHoverOverOverlayID = UNKNOWN_OVERLAY_ID;
};

#endif // hifi_Overlays_h
