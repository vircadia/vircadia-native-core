//
//  ProgramObject.h
//  interface
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ProgramObject__
#define __interface__ProgramObject__

#include <QGLShaderProgram>

#include <glm/glm.hpp>

class ProgramObject : public QGLShaderProgram {
public:
    
    ProgramObject(QObject* parent = 0);
    
    void setUniform(int location, const glm::vec3& value);
    void setUniform(const char* name, const glm::vec3& value);
};

#endif /* defined(__interface__ProgramObject__) */
