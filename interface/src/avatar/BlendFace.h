//
//  BlendFace.h
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__BlendFace__
#define __interface__BlendFace__

#include <QObject>
#include <QUrl>

#include "InterfaceConfig.h"
#include "renderer/GeometryCache.h"
#include "renderer/ProgramObject.h"
#include "renderer/TextureCache.h"

class QNetworkReply;

class Head;

/// A face formed from a linear mix of blendshapes according to a set of coefficients.
class BlendFace : public QObject {
    Q_OBJECT
    
public:

    BlendFace(Head* owningHead);
    ~BlendFace();
    
    bool isActive() const { return _geometry && _geometry->isLoaded(); }
    
    void init();
    bool render(float alpha);
    
    Q_INVOKABLE void setModelURL(const QUrl& url);
    const QUrl& getModelURL() const { return _modelURL; }

    void getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;
    
private:
    
    void deleteGeometry();
    
    Head* _owningHead;
    
    QUrl _modelURL;
    
    QSharedPointer<NetworkGeometry> _geometry;
    
    QVector<GLuint> _blendedVertexBufferIDs;
    QVector<QSharedPointer<Texture> > _dilatedTextures;
    
    QVector<glm::vec3> _blendedVertices;
    QVector<glm::vec3> _blendedNormals;
    
    static ProgramObject _eyeProgram;
};

#endif /* defined(__interface__BlendFace__) */
