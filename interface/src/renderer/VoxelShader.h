//
//  VoxelShader.h
//  interface/src/renderer
//
//  Created by Brad Hefta-Gaub on 9/23/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelShader_h
#define hifi_VoxelShader_h

#include <QObject>

class ProgramObject;

/// A generic full screen glow effect.
class VoxelShader : public QObject {
    Q_OBJECT
    
public:
    VoxelShader();
    ~VoxelShader();
    
    void init();
    
    /// Starts using the voxel geometry shader effect.
    void begin();
    
    /// Stops using the voxel geometry shader effect.
    void end();
    
    /// Gets access to attributes from the shader program
    int attributeLocation(const char * name) const;

    static ProgramObject* createGeometryShaderProgram(const QString& name);
    
public slots:
    
private:
    
    bool _initialized;

    ProgramObject* _program;
};

#endif // hifi_VoxelShader_h
