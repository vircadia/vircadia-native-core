//
//  FBXReader.h
//  libraries/fbx/src/
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OBJWriter.h"


bool writeOBJ(QString outFileName, QVector<model::Mesh>) {
//bool writeOBJ(QString outFileName, FBXGeometry& geometry, bool outputCentimeters, int whichMeshPart) {
    // QFile file(outFileName);
    // if (!file.open(QIODevice::WriteOnly)) {
    //     qWarning() << "unable to write to" << outFileName;
    //     return false;
    // }

    // QTextStream out(&file);
    // if (outputCentimeters) {
    //     out << "# This file uses centimeters as units\n\n";
    // }

    // unsigned int nth = 0;

    // // vertex indexes in obj files span the entire file
    // // vertex indexes in a mesh span just that mesh

    // int vertexIndexOffset = 0;

    // foreach (const FBXMesh& mesh, geometry.meshes) {
    //     bool verticesHaveBeenOutput = false;
    //     foreach (const FBXMeshPart &meshPart, mesh.parts) {
    //         if (whichMeshPart >= 0 && nth != (unsigned int) whichMeshPart) {
    //             nth++;
    //             continue;
    //         }

    //         if (!verticesHaveBeenOutput) {
    //             for (int i = 0; i < mesh.vertices.size(); i++) {
    //                 glm::vec4 v = mesh.modelTransform * glm::vec4(mesh.vertices[i], 1.0f);
    //                 out << "v ";
    //                 out << formatFloat(v[0]) << " ";
    //                 out << formatFloat(v[1]) << " ";
    //                 out << formatFloat(v[2]) << "\n";
    //             }
    //             verticesHaveBeenOutput = true;
    //         }

    //         out << "g hull-" << nth++ << "\n";
    //         int triangleCount = meshPart.triangleIndices.size() / 3;
    //         for (int i = 0; i < triangleCount; i++) {
    //             out << "f ";
    //             out << vertexIndexOffset + meshPart.triangleIndices[i*3] + 1 << " ";
    //             out << vertexIndexOffset + meshPart.triangleIndices[i*3+1] + 1  << " ";
    //             out << vertexIndexOffset + meshPart.triangleIndices[i*3+2] + 1  << "\n";
    //         }
    //         out << "\n";
    //     }

    //     if (verticesHaveBeenOutput) {
    //         vertexIndexOffset += mesh.vertices.size();
    //     }
    // }

    return true;
}
