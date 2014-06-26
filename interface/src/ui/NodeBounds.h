//
//  NodeBounds.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 05/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NodeBounds_h
#define hifi_NodeBounds_h

#include <QObject>

#include <NodeList.h>

const int MAX_OVERLAY_TEXT_LENGTH = 64;

class NodeBounds : public QObject {
    Q_OBJECT
public:
    NodeBounds(QObject* parent = NULL);

    bool getShowVoxelNodes() { return _showVoxelNodes; }
    bool getShowEntityNodes() { return _showEntityNodes; }
    bool getShowParticleNodes() { return _showParticleNodes; }

    void draw();
    void drawOverlay();

public slots:
    void setShowVoxelNodes(bool value) { _showVoxelNodes = value; }
    void setShowEntityNodes(bool value) { _showEntityNodes = value; }
    void setShowParticleNodes(bool value) { _showParticleNodes = value; }

protected:
    void drawNodeBorder(const glm::vec3& center, float scale, float red, float green, float blue);
    void getColorForNodeType(NodeType_t nodeType, float& red, float& green, float& blue);

private:
    bool _showVoxelNodes;
    bool _showEntityNodes;
    bool _showParticleNodes;
    char _overlayText[MAX_OVERLAY_TEXT_LENGTH + 1];

};

#endif // hifi_NodeBounds_h
