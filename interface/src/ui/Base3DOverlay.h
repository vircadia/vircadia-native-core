//
//  Base3DOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Base3DOverlay__
#define __interface__Base3DOverlay__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRect>
#include <QScriptValue>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "Overlay.h"

class Base3DOverlay : public Overlay {
    Q_OBJECT
    
public:
    Base3DOverlay();
    ~Base3DOverlay();

    // getters
    const glm::vec3& getPosition() const { return _position; }
    float getSize() const { return _size; }
    float getLineWidth() const { return _lineWidth; }
    bool getIsSolid() const { return _isSolid; }

    // setters
    void setPosition(const glm::vec3& position) { _position = position; }
    void setSize(float size) { _size = size; }
    void setLineWidth(float lineWidth) { _lineWidth = lineWidth; }
    void setIsSolid(bool isSolid) { _isSolid = isSolid; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    glm::vec3 _position;
    float _size;
    float _lineWidth;
    bool _isSolid;
};

 
#endif /* defined(__interface__Base3DOverlay__) */
