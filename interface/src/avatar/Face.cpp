//
//  Face.cpp
//  interface
//
//  Created by Andrzej Kapolka on 7/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/quaternion.hpp>

#include "Head.h"
#include "Face.h"
#include "renderer/ProgramObject.h"

using namespace cv;

ProgramObject* Face::_program = 0;
int Face::_texCoordCornerLocation;
int Face::_texCoordRightLocation;
int Face::_texCoordUpLocation;
GLuint Face::_vboID;

Face::Face(Head* owningHead) : _owningHead(owningHead), _colorTextureID(0), _depthTextureID(0) {
}

bool Face::render(float alpha) {
    if (_colorTextureID == 0 || _textureRect.size.area() == 0) {
        return false;
    }
    glPushMatrix();
    
    glTranslatef(_owningHead->getPosition().x, _owningHead->getPosition().y, _owningHead->getPosition().z);
    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 axis = glm::axis(orientation);
    //glRotatef(glm::angle(orientation), axis.x, axis.y, axis.z);
    glScalef(_owningHead->getScale(), _owningHead->getScale(), _owningHead->getScale());

    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    
    Point2f points[4];
    _textureRect.points(points);
    
    float aspect = _textureRect.size.height / _textureRect.size.width;
    
    if (_depthTextureID != 0) {
        const int VERTEX_WIDTH = 100;
        const int VERTEX_HEIGHT = 100;
        const int VERTEX_COUNT = VERTEX_WIDTH * VERTEX_HEIGHT;
        const int ELEMENTS_PER_VERTEX = 2;
        const int BUFFER_ELEMENTS = VERTEX_COUNT * ELEMENTS_PER_VERTEX;
        
        if (_program == 0) {
            _program = new ProgramObject();
            _program->addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/face.vert");
            _program->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/face.frag");
            _program->link();
            
            _program->bind();
            _program->setUniformValue("depthTexture", 0);
            _program->setUniformValue("colorTexture", 1);
            _program->release();
            
            _texCoordCornerLocation = _program->uniformLocation("texCoordCorner");
            _texCoordRightLocation = _program->uniformLocation("texCoordRight");
            _texCoordUpLocation = _program->uniformLocation("texCoordUp");
            
            glGenBuffers(1, &_vboID);
            glBindBuffer(GL_ARRAY_BUFFER, _vboID);
            float* vertices = new float[BUFFER_ELEMENTS];
            float* vertexPosition = vertices;
            for (int i = 0; i < VERTEX_HEIGHT; i++) {
                for (int j = 0; j < VERTEX_WIDTH; j++) {
                    *vertexPosition++ = j / (float)(VERTEX_WIDTH - 1);
                    *vertexPosition++ = i / (float)(VERTEX_HEIGHT - 1);
                }
            }
            glBufferData(GL_ARRAY_BUFFER, BUFFER_ELEMENTS * sizeof(float), vertices, GL_STATIC_DRAW);
            delete[] vertices;
            
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, _vboID);
        }
        glBindTexture(GL_TEXTURE_2D, _depthTextureID);
        
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        
        _program->bind();
        _program->setUniformValue(_texCoordCornerLocation, 
            points[0].x / _textureSize.width, points[0].y / _textureSize.height);
        _program->setUniformValue(_texCoordRightLocation,
            (points[3].x - points[0].x) / _textureSize.width, (points[3].y - points[0].y) / _textureSize.height);
        _program->setUniformValue(_texCoordUpLocation,
            (points[1].x - points[0].x) / _textureSize.width, (points[1].y - points[0].y) / _textureSize.height);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, 0);
        glPointSize(3.0f);
        glDrawArrays(GL_POINTS, 0, VERTEX_COUNT);
        glPointSize(1.0f);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        _program->release();
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    
    } else {
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glEnable(GL_TEXTURE_2D);
        
        glBegin(GL_QUADS);
        glTexCoord2f(points[0].x / _textureSize.width, points[0].y / _textureSize.height);
        glVertex3f(0.5f, 0, 0);    
        glTexCoord2f(points[1].x / _textureSize.width, points[1].y / _textureSize.height);
        glVertex3f(0.5f, aspect, 0);
        glTexCoord2f(points[2].x / _textureSize.width, points[2].y / _textureSize.height);
        glVertex3f(-0.5f, aspect, 0);
        glTexCoord2f(points[3].x / _textureSize.width, points[3].y / _textureSize.height);
        glVertex3f(-0.5f, 0, 0);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glPopMatrix();
    
    return true;
}

