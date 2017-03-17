//
//  OBJWriter.h
//  libraries/fbx/src/
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_objwriter_h
#define hifi_objwriter_h


#include <QString>
#include <QList>
#include <model/Geometry.h>

using MeshPointer = std::shared_ptr<model::Mesh>;

bool writeOBJToTextStream(QTextStream& out, QList<MeshPointer> meshes);
bool writeOBJToFile(QString path, QList<MeshPointer> meshes);
QString writeOBJToString(QList<MeshPointer> meshes);

#endif // hifi_objwriter_h
