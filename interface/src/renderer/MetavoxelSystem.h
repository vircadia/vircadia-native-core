//
//  MetavoxelSystem.h
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelSystem__
#define __interface__MetavoxelSystem__

#include <QOpenGLBuffer>
#include <QScriptEngine>
#include <QVector>

#include <glm/glm.hpp>

#include <MetavoxelData.h>

#include "ProgramObject.h"

/// Renders a metavoxel tree.
class MetavoxelSystem {
public:

    MetavoxelSystem();

    void init();
    
    void simulate(float deltaTime);
    void render();
    
private:

    class Point {
    public:
        glm::vec4 vertex;
        quint8 color[4];
        quint8 normal[3];
    };
    
    class PointVisitor : public MetavoxelVisitor {
    public:
        PointVisitor(QVector<Point>& points);
        virtual bool visit(const MetavoxelInfo& info);
    
    private:
        QVector<Point>& _points;
    };
    
    static ProgramObject _program;
    static int _pointScaleLocation;
    
    QScriptEngine _scriptEngine;
    MetavoxelData _data;
    QVector<Point> _points;
    PointVisitor _pointVisitor;
    QOpenGLBuffer _buffer;
};

#endif /* defined(__interface__MetavoxelSystem__) */
