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

#include <fsbinarystream.h>

#include "InterfaceConfig.h"

class Head;

/// A face formed from a linear mix of blendshapes according to a set of coefficients.
class BlendFace : public QObject {
    Q_OBJECT
    
public:

    BlendFace(Head* owningHead);
    ~BlendFace();
    
    bool render(float alpha);
    
public slots:
    
    void setRig(const fs::fsMsgRig& rig);
    
private:
    
    Head* _owningHead;
    
    GLuint _iboID;
    GLuint _vboID;
    
    GLsizei _quadIndexCount;
    GLsizei _triangleIndexCount;
    std::vector<fs::fsVector3f> _baseVertices;
    std::vector<fs::fsVertexData> _blendshapes;
    std::vector<fs::fsVector3f> _blendedVertices;
};

#endif /* defined(__interface__BlendFace__) */
