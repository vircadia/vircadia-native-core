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
#include "renderer/FBXReader.h"

class QNetworkReply;

class Head;

/// A face formed from a linear mix of blendshapes according to a set of coefficients.
class BlendFace : public QObject {
    Q_OBJECT
    
public:

    BlendFace(Head* owningHead);
    ~BlendFace();
    
    bool isActive() const { return !_meshIDs.isEmpty(); }
    
    bool render(float alpha);
    
    Q_INVOKABLE void setModelURL(const QUrl& url);
    const QUrl& getModelURL() const { return _modelURL; }

private slots:
    
    void handleModelDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleModelReplyError();
    
private:
    
    void setGeometry(const FBXGeometry& geometry);
    void deleteGeometry();
    
    Head* _owningHead;
    
    QUrl _modelURL;
    
    QNetworkReply* _modelReply;

    typedef QPair<GLuint, GLuint> VerticesIndices;
    QVector<VerticesIndices> _meshIDs;
    
    FBXGeometry _geometry;
    QVector<glm::vec3> _blendedVertices;
    QVector<glm::vec3> _blendedNormals;
};

#endif /* defined(__interface__BlendFace__) */
