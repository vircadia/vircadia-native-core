//
//  Line3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Line3DOverlay_h
#define hifi_Line3DOverlay_h

#include "Base3DOverlay.h"

class Line3DOverlay : public Base3DOverlay {
    Q_OBJECT
    
public:
    static QString const TYPE;
    virtual QString getType() const { return TYPE; }

    Line3DOverlay();
    Line3DOverlay(const Line3DOverlay* line3DOverlay);
    ~Line3DOverlay();
    virtual void render(RenderArgs* args);
    virtual const render::ShapeKey getShapeKey() override;
    virtual AABox getBounds() const;

    // getters
    const glm::vec3& getStart() const { return _start; }
    const glm::vec3& getEnd() const { return _end; }

    // setters
    void setStart(const glm::vec3& start) { _start = start; }
    void setEnd(const glm::vec3& end) { _end = end; }

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual Line3DOverlay* createClone() const;

protected:
    glm::vec3 _start;
    glm::vec3 _end;
    int _geometryCacheID;
};

 
#endif // hifi_Line3DOverlay_h
