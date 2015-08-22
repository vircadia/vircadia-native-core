//
//  Asset.h
//  libraries/model/src/model
//
//  Created by Sam Gateau on 08/21/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Asset_h
#define hifi_model_Asset_h

#include <memory>
#include <vector>

#include "Material.h"
#include "Geometry.h"

namespace model {

template <class T>
class Table {
public:
    typedef std::vector< T > Vector;
    typedef int ID;

    const ID INVALID_ID = 0;

    enum Version {
        DRAFT = 0,
        FINAL,
        NUM_VERSIONS,
    };

    Table() {
        for (auto e : _elements) {
            e.resize(0);
        }
    }
    ~Table() {}

    ID add(Version v, const T& element) {
        switch (v) {
        case DRAFT: {
            _elements[DRAFT].push_back(element);
            return ID(-(_elements[DRAFT].size() - 1));
            break;
        }
        case FINAL: {
            _elements[FINAL].push_back(element);
            return ID(_elements[FINAL].size() - 1);
            break;
        }
        }
        return INVALID_ID;
    }

protected:
    Vector _elements[NUM_VERSIONS];
};

typedef Table< MaterialPointer > MaterialTable;
typedef Table< MeshPointer > MeshTable;

class Asset {
public:    


    Asset();
    ~Asset();

    MeshTable& editMeshes() { return _meshes; }
    const MeshTable& getMeshes() const { return _meshes; }

    MaterialTable& editMaterials() { return _materials; }
    const MaterialTable& getMaterials() const { return _materials; }

protected:

    MeshTable _meshes;
    MaterialTable _materials;

};

typedef std::shared_ptr< Asset > AssetPointer;

};
#endif