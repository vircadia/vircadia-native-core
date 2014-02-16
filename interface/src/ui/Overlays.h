//
//  Overlays.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Overlays__
#define __interface__Overlays__

/**
#include <QGLWidget>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRect>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "InterfaceConfig.h"
**/

#include <QScriptValue>

#include "ImageOverlay.h"

class Overlays : public QObject {
    Q_OBJECT
public:
    Overlays();
    ~Overlays();
    void init(QGLWidget* parent);
    void render();

public slots:
    /// adds an overlay with the specific properties
    unsigned int addOverlay(const QScriptValue& properties);

    /// edits an overlay updating only the included properties, will return the identified OverlayID in case of
    /// successful edit, if the input id is for an unknown overlay this function will have no effect
    bool editOverlay(unsigned int id, const QScriptValue& properties);

    /// deletes a particle
    void deleteOverlay(unsigned int id);

private:
    QMap<unsigned int, ImageOverlay*> _imageOverlays;
    static unsigned int _nextOverlayID;
    QGLWidget* _parent;
};

 
#endif /* defined(__interface__Overlays__) */
