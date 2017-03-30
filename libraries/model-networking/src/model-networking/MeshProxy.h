//
//  MeshProxy.h
//  libraries/model/src/model/
//
//  Created by Seth Alves on 2017-1-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MeshProxy_h
#define hifi_MeshProxy_h

#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QtScript/QScriptValue>

#include <model/Geometry.h>

using MeshPointer = std::shared_ptr<model::Mesh>;

class MeshProxy : public QObject {
    Q_OBJECT

public:
    MeshProxy(MeshPointer mesh) : _mesh(mesh) {}
    ~MeshProxy() {}

    MeshPointer getMeshPointer() const { return _mesh; }

    Q_INVOKABLE int getNumVertices() const { return (int)_mesh->getNumVertices(); }
    Q_INVOKABLE glm::vec3 getPos3(int index) const { return _mesh->getPos3(index); }


protected:
    MeshPointer _mesh;
};

Q_DECLARE_METATYPE(MeshProxy*);

class MeshProxyList : public QList<MeshProxy*> {}; // typedef and using fight with the Qt macros/templates, do this instead
Q_DECLARE_METATYPE(MeshProxyList);


QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in);
void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out);

QScriptValue meshesToScriptValue(QScriptEngine* engine, const MeshProxyList &in);
void meshesFromScriptValue(const QScriptValue& value, MeshProxyList &out);

#endif // hifi_MeshProxy_h
