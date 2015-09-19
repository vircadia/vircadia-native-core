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

    static const ID INVALID_ID = 0;

    typedef size_t Index;
    enum Version {
        DRAFT = 0,
        FINAL,
        NUM_VERSIONS,
    };

    static Version evalVersionFromID(ID id) {
        if (id <= 0) {
            return DRAFT;
        } else {
            return FINAL;
        }
    }
    static Index evalIndexFromID(ID id) {
        return Index(id < 0 ? -id : id) - 1;
    }
    static ID evalID(Index index, Version version) {
        return (version == DRAFT ? -int(index + 1) : int(index + 1));
    }

    Table() {
        for (auto e : _elements) {
            e.resize(0);
        }
    }
    ~Table() {}

    Index getNumElements() const {
        return _elements[DRAFT].size();
    }

    ID add(const T& element) {
        for (auto e : _elements) {
            e.push_back(element);
        }
        return evalID(_elements[DRAFT].size(), DRAFT);
    }

    void set(ID id, const T& element) {
        Index index = evalIndexFromID(id);
        if (index < getNumElements()) {
            _elements[DRAFT][index] = element;
        }
    }

     const T& get(ID id, const T& element) const {
        Index index = evalIndexFromID(id);
        if (index < getNumElements()) {
            return _elements[DRAFT][index];
        }
        return _default;
    }

protected:
    Vector _elements[NUM_VERSIONS];
    T _default;
};

typedef Table< MaterialPointer > MaterialTable;

typedef Table< MeshPointer > MeshTable;


class Shape {
public:

    MeshTable::ID _meshID{ MeshTable::INVALID_ID };
    int _partID = 0;

    MaterialTable::ID _materialID{ MaterialTable::INVALID_ID };
};

typedef Table< Shape > ShapeTable;

class Asset {
public:    


    Asset();
    ~Asset();

    MeshTable& editMeshes() { return _meshes; }
    const MeshTable& getMeshes() const { return _meshes; }

    MaterialTable& editMaterials() { return _materials; }
    const MaterialTable& getMaterials() const { return _materials; }

    ShapeTable& editShapes() { return _shapes; }
    const ShapeTable& getShapes() const { return _shapes; }

protected:

    MeshTable _meshes;
    MaterialTable _materials;
    ShapeTable _shapes;

};

typedef std::shared_ptr< Asset > AssetPointer;

};
#endif