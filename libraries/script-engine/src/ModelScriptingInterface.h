//
//  ModelScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ModelScriptingInterface_h
#define hifi_ModelScriptingInterface_h

#include <QtCore/QObject>
#include <QScriptValue>
#include <OBJWriter.h>
#include <model/Geometry.h>

using MeshPointer = std::shared_ptr<model::Mesh>;

class ModelScriptingInterface : public QObject {
    Q_OBJECT

public:
    ModelScriptingInterface(QObject* parent);
};

class MeshProxy : public QObject {
    Q_OBJECT

public:
    MeshProxy(MeshPointer mesh);
    // MeshProxy(const MeshProxy& meshProxy) { _mesh = meshProxy.getMeshPointer(); }
    ~MeshProxy();

    MeshPointer getMeshPointer() const { return _mesh; }

    Q_INVOKABLE int getNumVertices() const { return _mesh->getNumVertices(); }

protected:
    MeshPointer _mesh;
};


Q_DECLARE_METATYPE(MeshProxy*);


QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in);
// QScriptValue meshToScriptValue(QScriptEngine* engine, const MeshPointer& in);
void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out);

#endif // hifi_ModelScriptingInterface_h
