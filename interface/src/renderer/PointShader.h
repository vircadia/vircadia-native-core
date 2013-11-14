//
//  PointShader.h
//  interface
//
//  Created by Brad Hefta-Gaub on 10/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__PointShader__
#define __interface__PointShader__

#include <QObject>

class ProgramObject;

/// A shader program that draws voxels as points with variable sizes
class PointShader : public QObject {
    Q_OBJECT
    
public:
    PointShader();
    ~PointShader();
    
    void init();
    
    /// Starts using the voxel point shader program.
    void begin();
    
    /// Stops using the voxel point shader program.
    void end();
    
    /// Gets access to attributes from the shader program
    int attributeLocation(const char* name) const;
    int uniformLocation(const char* name) const;
    void setUniformValue(int uniformLocation, float value);
    void setUniformValue(int uniformLocation, const glm::vec3& value);

    static ProgramObject* createPointShaderProgram(const QString& name);
    
private:
    bool _initialized;
    ProgramObject* _program;
};

#endif /* defined(__interface__PointShader__) */
