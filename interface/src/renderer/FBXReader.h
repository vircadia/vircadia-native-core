//
//  FBXReader.h
//  interface
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__FBXReader__
#define __interface__FBXReader__

#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>

class QIODevice;

class FBXNode;

typedef QList<FBXNode> FBXNodeList;

/// A node within an FBX document.
class FBXNode {
public:
    
    QByteArray name;
    QVariantList properties;
    FBXNodeList children;
};

/// A single blendshape extracted from an FBX document.
class FBXBlendshape {
public:
    
    QVector<int> indices;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
};

/// A single mesh (with optional blendshapes) extracted from an FBX document.
class FBXMesh {
public:
    
    QVector<int> quadIndices;
    QVector<int> triangleIndices;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
    
    QVector<FBXBlendshape> blendshapes;
};

/// A set of meshes extracted from an FBX document.
class FBXGeometry {
public:

    QVector<FBXMesh> meshes;
};

/// Parses the input from the supplied data as an FBX file.
/// \exception QString if an error occurs in parsing
FBXNode parseFBX(const QByteArray& data);

/// Parses the input from the supplied device as an FBX file.
/// \exception QString if an error occurs in parsing
FBXNode parseFBX(QIODevice* device);

/// Extracts the geometry from a parsed FBX node.
FBXGeometry extractFBXGeometry(const FBXNode& node);

void printNode(const FBXNode& node, int indent = 0);

#endif /* defined(__interface__FBXReader__) */
