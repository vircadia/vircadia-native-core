//
//  NodeBounds.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 05/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <NodeList.h>
#include <QGLWidget>

#include "Application.h"
#include "NodeBounds.h"
#include "Util.h"

NodeBounds::NodeBounds(QObject* parent) :
    QObject(parent),
    _showVoxelNodes(false),
    _showModelNodes(false),
    _showParticleNodes(false),
    _overlayText("") {

}

void NodeBounds::draw() {
    NodeToJurisdictionMap& voxelServerJurisdictions = Application::getInstance()->getVoxelServerJurisdictions();
    NodeToJurisdictionMap& modelServerJurisdictions = Application::getInstance()->getModelServerJurisdictions();
    NodeToJurisdictionMap& particleServerJurisdictions = Application::getInstance()->getParticleServerJurisdictions();
    NodeToJurisdictionMap* serverJurisdictions;

    Application* application = Application::getInstance();
    const glm::vec3& mouseRayOrigin = application->getMouseRayOrigin();
    const glm::vec3& mouseRayDirection = application->getMouseRayDirection();

    float closest = FLT_MAX;
    glm::vec3 closestCenter;
    float closestScale = 0;
    bool closestIsInside = true;
    Node* closestNode = NULL;
    bool isSelecting = false;

    int num = 0;
    NodeList* nodeList = NodeList::getInstance();

    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        NodeType_t nodeType = node->getType();

        float r = nodeType == NodeType::VoxelServer ? 1.0 : 0.0; 
        float g = nodeType == NodeType::ParticleServer ? 1.0 : 0.0; 
        float b = nodeType == NodeType::ModelServer ? 1.0 : 0.0; 

        if (nodeType == NodeType::VoxelServer && _showVoxelNodes) {
            serverJurisdictions = &voxelServerJurisdictions;
        } else if (nodeType == NodeType::ModelServer && _showModelNodes) {
            serverJurisdictions = &modelServerJurisdictions;
        } else if (nodeType == NodeType::ParticleServer && _showParticleNodes) {
            serverJurisdictions = &particleServerJurisdictions;
        } else {
            continue;
        }
        
        QUuid nodeUUID = node->getUUID();
        if (serverJurisdictions->find(nodeUUID) != serverJurisdictions->end()) {
            const JurisdictionMap& map = serverJurisdictions->value(nodeUUID);
            
            unsigned char* rootCode = map.getRootOctalCode();
            
            if (rootCode) {
                VoxelPositionSize rootDetails;
                voxelDetailsForCode(rootCode, rootDetails);
                glm::vec3 location(rootDetails.x, rootDetails.y, rootDetails.z);
                location *= (float)TREE_SCALE;
                float len = rootDetails.s * TREE_SCALE;
                AABox serverBounds(location, rootDetails.s * TREE_SCALE);

                glm::vec3 center = serverBounds.getVertex(BOTTOM_RIGHT_NEAR) + (serverBounds.getVertex(TOP_LEFT_FAR) - serverBounds.getVertex(BOTTOM_RIGHT_NEAR)) / 2.0f;

                float scaleFactor = rootDetails.s * TREE_SCALE;
                scaleFactor *= rootDetails.s == 1 ? 1.05 : 0.95;
                scaleFactor *= nodeType == NodeType::VoxelServer ? 1.0 : (nodeType == NodeType::ParticleServer ? 0.980 : 0.960); 

                drawNodeBorder(center, scaleFactor, r, g, b);

                float distance;
                BoxFace face;
                bool inside = serverBounds.contains(mouseRayOrigin);
                bool colliding = serverBounds.findRayIntersection(mouseRayOrigin, mouseRayDirection, distance, face);

                if (colliding && (!isSelecting || (!inside && (distance < closest || closestIsInside)))) {
                    closest = distance;
                    closestCenter = center;
                    closestScale = scaleFactor;
                    closestIsInside = inside;
                    closestNode = node.data();
                    isSelecting = true;
                }
            }
        }
    }

    if (isSelecting) {
        glPushMatrix();

        glTranslatef(closestCenter.x, closestCenter.y, closestCenter.z);
        glScalef(closestScale, closestScale, closestScale);

        NodeType_t selectedNodeType = closestNode->getType();
        float r = selectedNodeType == NodeType::VoxelServer ? 1.0 : 0.0; 
        float g = selectedNodeType == NodeType::ParticleServer ? 1.0 : 0.0; 
        float b = selectedNodeType == NodeType::ModelServer ? 1.0 : 0.0; 
        glColor4f(r, g, b, 0.2);
        glutSolidCube(1.0);

        glPopMatrix();

        HifiSockAddr addr = closestNode->getPublicSocket();
        _overlayText = QString("%1:%2")
            .arg(addr.getAddress().toString())
            .arg(addr.getPort());
    } else {
        _overlayText = QString();
    }
}

void NodeBounds::drawNodeBorder(glm::vec3 center, float scale, float red, float green, float blue) {
    glPushMatrix();

    glTranslatef(center.x, center.y, center.z);
    glScalef(scale, scale, scale);

    glLineWidth(2.5); 
    glColor3f(red, green, blue);
    glBegin(GL_LINES);

    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f( 0.5, -0.5, -0.5);

    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5,  0.5, -0.5);

    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5, -0.5,  0.5);

    glVertex3f(-0.5,  0.5, -0.5);
    glVertex3f( 0.5,  0.5, -0.5);

    glVertex3f(-0.5,  0.5, -0.5);
    glVertex3f(-0.5,  0.5,  0.5);



    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f(-0.5,  0.5,  0.5);

    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f( 0.5, -0.5,  0.5);

    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f( 0.5,  0.5, -0.5);

    glVertex3f( 0.5, -0.5,  0.5);
    glVertex3f(-0.5, -0.5,  0.5);

    glVertex3f( 0.5, -0.5,  0.5);
    glVertex3f( 0.5, -0.5, -0.5);


    glVertex3f( 0.5,  0.5, -0.5);
    glVertex3f( 0.5, -0.5, -0.5);

    glVertex3f(-0.5,  0.5,  0.5);
    glVertex3f(-0.5, -0.5,  0.5);

    glEnd();

    glPopMatrix();
}

const float WHITE_TEXT[] = { 0.93f, 0.93f, 0.93f };
void NodeBounds::drawOverlay() {
    if (!_overlayText.isNull() && !_overlayText.isEmpty()) {
        QGLWidget* glWidget = Application::getInstance()->getGLWidget();
        Application* application = Application::getInstance();
        drawText(application->getMouseX(), application->getMouseY(), 0.1f, 0.0f, 0, _overlayText.toLocal8Bit().data(), WHITE_TEXT);
        // drawText(application->getMouseX(), application->getMouseY(), 0.1f, 0.0f, 0, "durr", WHITE_TEXT);
    }
}
