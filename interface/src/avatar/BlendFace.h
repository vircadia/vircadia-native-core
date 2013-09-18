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

#include <fsbinarystream.h>

#include "InterfaceConfig.h"

class QNetworkReply;

class Head;

/// A face formed from a linear mix of blendshapes according to a set of coefficients.
class BlendFace : public QObject {
    Q_OBJECT
    
public:

    BlendFace(Head* owningHead);
    ~BlendFace();
    
    bool render(float alpha);
    
    Q_INVOKABLE void setModelURL(const QUrl& url);
    const QUrl& getModelURL() const { return _modelURL; }
    
public slots:
    
    void setRig(const fs::fsMsgRig& rig);

private slots:
    
    void handleModelDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleModelReplyError();
    
private:
    
    Head* _owningHead;
    
    QUrl _modelURL;
    
    QNetworkReply* _modelReply;

    GLuint _iboID;
    GLuint _vboID;
    
    GLsizei _quadIndexCount;
    GLsizei _triangleIndexCount;
    std::vector<fs::fsVector3f> _baseVertices;
    std::vector<fs::fsVertexData> _blendshapes;
    std::vector<fs::fsVector3f> _blendedVertices;
};

#endif /* defined(__interface__BlendFace__) */
