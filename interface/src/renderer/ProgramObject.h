//
//  ProgramObject.h
//  interface
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ProgramObject__
#define __interface__ProgramObject__

#include <QObject>

#include <glm/glm.hpp>

#include "InterfaceConfig.h"

class ShaderObject;

class ProgramObject {
public:
    
    ProgramObject();
    ~ProgramObject();

    GLhandleARB getHandle() const { return _handle; }
    
    void attach(ShaderObject* shader);
    bool attachFromSourceCode(int type, const char* source);
    bool attachFromSourceFile(int type, const char* filename);

    bool link();

    QByteArray getLog() const;

    void bind() const;
    void release() const;

    int getUniformLocation(const char* name) const;
    
    void setUniform(int location, int value);
    void setUniform(const char* name, int value);
    
    void setUniform(int location, float value);
    void setUniform(const char* name, float value);

    void setUniform(int location, float x, float y);
    void setUniform(const char* name, float x, float y);

    void setUniform(int location, const glm::vec3& value);
    void setUniform(const char* name, const glm::vec3& value);
    
    void setUniform(int location, float x, float y, float z);
    void setUniform(const char* name, float x, float y, float z);
    
    void setUniform(int location, float x, float y, float z, float w);
    void setUniform(const char* name, float x, float y, float z, float w);
    
private:
    
    Q_DISABLE_COPY(ProgramObject)
    
    GLhandleARB _handle;
};

#endif /* defined(__interface__ProgramObject__) */
