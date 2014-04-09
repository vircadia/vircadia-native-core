//
//  PointShader.h
//  interface/src/renderer
//
//  Created by Brad Hefta-Gaub on 10/30/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PointShader_h
#define hifi_PointShader_h

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

#endif // hifi_PointShader_h
