//
//  ProgramObject.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ProgramObject_h
#define hifi_ProgramObject_h

#include <QGLShaderProgram>

#include <glm/glm.hpp>

class ProgramObject : public QGLShaderProgram {
public:
    
    ProgramObject(QObject* parent = 0);
    
    void setUniform(int location, const glm::vec2& value);
    void setUniform(const char* name, const glm::vec2& value);
    void setUniform(int location, const glm::vec3& value);
    void setUniform(const char* name, const glm::vec3& value);
    void setUniform(int location, const glm::vec4& value);
    void setUniform(const char* name, const glm::vec4& value);
    void setUniformArray(const char* name, const glm::vec3* values, int count); 
};

#endif // hifi_ProgramObject_h
