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

#include <QReadWriteLock>
#include <QScriptValue>

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

    void cleanupAllOverlays();

public slots:
    /// adds an overlay with the specific properties
    unsigned int addOverlay(const QString& type, const QVariant& properties);

    /// adds an overlay that's already been created
    unsigned int addOverlay(Overlay* overlay) { return addOverlay(Overlay::Pointer(overlay)); }
    unsigned int addOverlay(Overlay::Pointer overlay);

    /// clones an existing overlay
    unsigned int cloneOverlay(unsigned int id);

    /// edits an overlay updating only the included properties, will return the identified OverlayID in case of
    /// successful edit, if the input id is for an unknown overlay this function will have no effect
    bool editOverlay(unsigned int id, const QVariant& properties);

    /// edits an overlay updating only the included properties, will return the identified OverlayID in case of
    /// successful edit, if the input id is for an unknown overlay this function will have no effect
    bool editOverlays(const QVariant& propertiesById);

    /// deletes a particle
    void deleteOverlay(unsigned int id);

    /// get the string type of the overlay used in addOverlay
    QString getOverlayType(unsigned int overlayId) const;

    unsigned int getParentPanel(unsigned int childId) const;
    void setParentPanel(unsigned int childId, unsigned int panelId);

    /// returns the top most 2D overlay at the screen point, or 0 if not overlay at that point
    unsigned int getOverlayAtPoint(const glm::vec2& point);

    /// returns the value of specified property, or null if there is no such property
    OverlayPropertyResult getProperty(unsigned int id, const QString& property);

    /// returns details about the closest 3D Overlay hit by the pick ray
    RayToOverlayIntersectionResult findRayIntersection(const PickRay& ray);

    /// returns whether the overlay's assets are loaded or not
    bool isLoaded(unsigned int id);

    /// returns the size of the given text in the specified overlay if it is a text overlay: in pixels if it is a 2D text
    /// overlay; in meters if it is a 3D text overlay
    QSizeF textSize(unsigned int id, const QString& text) const;

    // Return the size of the virtual screen
    float width() const;
    float height() const;


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

    /// return true if there is an overlay with that id else false
    bool isAddedOverlay(unsigned int id);

    /// return true if there is a panel with that id else false
    bool isAddedPanel(unsigned int id) { return _panels.contains(id); }

signals:
    void overlayDeleted(unsigned int id);
    void panelDeleted(unsigned int id);

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
};



#endif // hifi_Overlays_h
