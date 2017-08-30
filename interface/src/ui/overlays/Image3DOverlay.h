//
//  Image3DOverlay.h
//
//
//  Created by Clement on 7/1/14.
//  Modified and renamed by Zander Otavka on 8/4/15
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Image3DOverlay_h
#define hifi_Image3DOverlay_h

#include <TextureCache.h>

#include "Billboard3DOverlay.h"

class Image3DOverlay : public Billboard3DOverlay {
    Q_OBJECT

public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Image3DOverlay();
    Image3DOverlay(const Image3DOverlay* image3DOverlay);
    ~Image3DOverlay();
    virtual void render(RenderArgs* args) override;

    virtual void update(float deltatime) override;

    virtual const render::ShapeKey getShapeKey() override;

    // setters
    void setURL(const QString& url);
    void setClipFromSource(const QRect& bounds) { _fromImage = bounds; }

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;
    bool isTransparent() override { return Base3DOverlay::isTransparent() || _alphaTexture; }

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                        BoxFace& face, glm::vec3& surfaceNormal) override;

    virtual Image3DOverlay* createClone() const override;

private:
    QString _url;
    NetworkTexturePointer _texture;
    bool _textureIsLoaded { false };
    bool _alphaTexture { false };
    bool _emissive { false };

    QRect _fromImage; // where from in the image to sample
    int _geometryId { 0 };
};

#endif // hifi_Image3DOverlay_h
