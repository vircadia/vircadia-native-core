//
//  Overlays.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Overlays__
#define __interface__Overlays__

#include <QScriptValue>

#include "Overlay.h"

class Overlays : public QObject {
    Q_OBJECT
public:
    Overlays();
    ~Overlays();
    void init(QGLWidget* parent);
    void render3D();
    void render2D();

public slots:
    /// adds an overlay with the specific properties
    unsigned int addOverlay(const QString& type, const QScriptValue& properties);

    /// edits an overlay updating only the included properties, will return the identified OverlayID in case of
    /// successful edit, if the input id is for an unknown overlay this function will have no effect
    bool editOverlay(unsigned int id, const QScriptValue& properties);

    /// deletes a particle
    void deleteOverlay(unsigned int id);

    /// returns the top most overlay at the screen point, or 0 if not overlay at that point
    unsigned int getOverlayAtPoint(const glm::vec2& point);

private:
    QMap<unsigned int, Overlay*> _overlays2D;
    QMap<unsigned int, Overlay*> _overlays3D;
    static unsigned int _nextOverlayID;
    QGLWidget* _parent;
};

 
#endif /* defined(__interface__Overlays__) */
