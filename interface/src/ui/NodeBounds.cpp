//
//  NodeBounds.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 05/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This class draws a border around the different Voxel, Entity, and Particle nodes on the current domain,
//  and a semi-transparent cube around the currently mouse-overed node.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "Util.h"

#include "NodeBounds.h"

NodeBounds::NodeBounds(QObject* parent) :
    QObject(parent),
    _showVoxelNodes(false),
    _showEntityNodes(false),
    _showParticleNodes(false),
    _overlayText() {

}

void NodeBounds::draw() {
    if (!(_showVoxelNodes || _showEntityNodes || _showParticleNodes)) {
        _overlayText[0] = '\0';
        return;
    }

    NodeToJurisdictionMap& voxelServerJurisdictions = Application::getInstance()->getVoxelServerJurisdictions();
    NodeToJurisdictionMap& entityServerJurisdictions = Application::getInstance()->getEntityServerJurisdictions();
    NodeToJurisdictionMap& particleServerJurisdictions = Application::getInstance()->getParticleServerJurisdictions();
    NodeToJurisdictionMap* serverJurisdictions;

    // Compute ray to find selected nodes later on.  We can't use the pre-computed ray in Application because it centers
    // itself after the cursor disappears.
    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();
    float mouseX = application->getMouseX() / (float)glWidget->getDeviceWidth();
    float mouseY = application->getMouseY() / (float)glWidget->getDeviceHeight();
    glm::vec3 mouseRayOrigin;
    glm::vec3 mouseRayDirection;
    application->getViewFrustum()->computePickRay(mouseX, mouseY, mouseRayOrigin, mouseRayDirection);

    // Variables to keep track of the selected node and properties to draw the cube later if needed
    Node* selectedNode = NULL;
    float selectedDistance = FLT_MAX;
    bool selectedIsInside = true;
    glm::vec3 selectedCenter;
    float selectedScale = 0;

    NodeList* nodeList = NodeList::getInstance();

    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        NodeType_t nodeType = node->getType();

        if (nodeType == NodeType::VoxelServer && _showVoxelNodes) {
            serverJurisdictions = &voxelServerJurisdictions;
        } else if (nodeType == NodeType::EntityServer && _showEntityNodes) {
            serverJurisdictions = &entityServerJurisdictions;
        } else if (nodeType == NodeType::ParticleServer && _showParticleNodes) {
            serverJurisdictions = &particleServerJurisdictions;
        } else {
            continue;
        }

        QUuid nodeUUID = node->getUUID();
        serverJurisdictions->lockForRead();
        if (serverJurisdictions->find(nodeUUID) != serverJurisdictions->end()) {
            const JurisdictionMap& map = (*serverJurisdictions)[nodeUUID];

            unsigned char* rootCode = map.getRootOctalCode();

            if (rootCode) {
                VoxelPositionSize rootDetails;
                voxelDetailsForCode(rootCode, rootDetails);
                serverJurisdictions->unlock();
                glm::vec3 location(rootDetails.x, rootDetails.y, rootDetails.z);
                location *= (float)TREE_SCALE;

                AACube serverBounds(location, rootDetails.s * TREE_SCALE);

                glm::vec3 center = serverBounds.getVertex(BOTTOM_RIGHT_NEAR)
                    + ((serverBounds.getVertex(TOP_LEFT_FAR) - serverBounds.getVertex(BOTTOM_RIGHT_NEAR)) / 2.0f);

                const float VOXEL_NODE_SCALE = 1.00f;
                const float ENTITY_NODE_SCALE = 0.99f;
                const float PARTICLE_NODE_SCALE = 0.98f;

                float scaleFactor = rootDetails.s * TREE_SCALE;

                // Scale by 0.92 - 1.00 depending on the scale of the node.  This allows smaller nodes to scale in
                // a bit and not overlap larger nodes.
                scaleFactor *= 0.92 + (rootDetails.s * 0.08);

                // Scale different node types slightly differently because it's common for them to overlap.
                if (nodeType == NodeType::VoxelServer) {
                    scaleFactor *= VOXEL_NODE_SCALE;
                } else if (nodeType == NodeType::EntityServer) {
                    scaleFactor *= ENTITY_NODE_SCALE;
                } else {
                    scaleFactor *= PARTICLE_NODE_SCALE;
                }

                float red, green, blue;
                getColorForNodeType(nodeType, red, green, blue);
                drawNodeBorder(center, scaleFactor, red, green, blue);

                float distance;
                BoxFace face;
                bool inside = serverBounds.contains(mouseRayOrigin);
                bool colliding = serverBounds.findRayIntersection(mouseRayOrigin, mouseRayDirection, distance, face);

                // If the camera is inside a node it will be "selected" if you don't have your cursor over another node
                // that you aren't inside.
                if (colliding && (!selectedNode || (!inside && (distance < selectedDistance || selectedIsInside)))) {
                    selectedNode = node.data();
                    selectedDistance = distance;
                    selectedIsInside = inside;
                    selectedCenter = center;
                    selectedScale = scaleFactor;
                }
            } else {
                serverJurisdictions->unlock();
            }
        } else {
            serverJurisdictions->unlock();
        }
    }

    if (selectedNode) {
        glPushMatrix();

        glTranslatef(selectedCenter.x, selectedCenter.y, selectedCenter.z);
        glScalef(selectedScale, selectedScale, selectedScale);

        float red, green, blue;
        getColorForNodeType(selectedNode->getType(), red, green, blue);

        glColor4f(red, green, blue, 0.2f);
        glutSolidCube(1.0);

        glPopMatrix();

        HifiSockAddr addr = selectedNode->getPublicSocket();
        QString overlay = QString("%1:%2  %3ms")
            .arg(addr.getAddress().toString())
            .arg(addr.getPort())
            .arg(selectedNode->getPingMs())
            .left(MAX_OVERLAY_TEXT_LENGTH);

        // Ideally we'd just use a QString, but I ran into weird blinking issues using
        // constData() directly, as if the data was being overwritten.
        strcpy(_overlayText, overlay.toLocal8Bit().constData());
    } else {
        _overlayText[0] = '\0';
    }
}

void NodeBounds::drawNodeBorder(const glm::vec3& center, float scale, float red, float green, float blue) {
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

void NodeBounds::getColorForNodeType(NodeType_t nodeType, float& red, float& green, float& blue) {
    red = nodeType == NodeType::VoxelServer ? 1.0 : 0.0;
    green = nodeType == NodeType::ParticleServer ? 1.0 : 0.0;
    blue = nodeType == NodeType::EntityServer ? 1.0 : 0.0;
}

void NodeBounds::drawOverlay() {
    if (strlen(_overlayText) > 0) {
        Application* application = Application::getInstance();

        const float TEXT_COLOR[] = { 0.90f, 0.90f, 0.90f };
        const float TEXT_SCALE = 0.1f;
        const int TEXT_HEIGHT = 10;
        const float ROTATION = 0.0f;
        const int FONT = 2;
        const int PADDING = 10;
        const int MOUSE_OFFSET = 10;
        const int BACKGROUND_BEVEL = 3;

        int mouseX = application->getMouseX(),
            mouseY = application->getMouseY(),
            textWidth = widthText(TEXT_SCALE, 0, _overlayText);
        glColor4f(0.4f, 0.4f, 0.4f, 0.6f);
        renderBevelCornersRect(mouseX + MOUSE_OFFSET, mouseY - TEXT_HEIGHT - PADDING,
                               textWidth + (2 * PADDING), TEXT_HEIGHT + (2 * PADDING), BACKGROUND_BEVEL);
        drawText(mouseX + MOUSE_OFFSET + PADDING, mouseY, TEXT_SCALE, ROTATION, FONT, _overlayText, TEXT_COLOR);
    }
}
