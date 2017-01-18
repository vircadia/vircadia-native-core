//
//  CauterizeableModel.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2016.01.17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CauterizedModel_h
#define hifi_CauterizedModel_h


#include <Model.h>

class CauterizedModel : public Model {
    Q_OBJECT

public:
    CauterizedModel(RigPointer rig, QObject* parent);
    virtual ~CauterizedModel();

	void deleteGeometry() override;
    virtual void updateRig(float deltaTime, glm::mat4 parentTransform) override;

    void setCauterizeBones(bool flag) { _cauterizeBones = flag; }
    bool getCauterizeBones() const { return _cauterizeBones; }

    const std::unordered_set<int>& getCauterizeBoneSet() const { return _cauterizeBoneSet; }
    void setCauterizeBoneSet(const std::unordered_set<int>& boneSet) { _cauterizeBoneSet = boneSet; }

	void createVisibleRenderItemSet() override;
    void createCollisionRenderItemSet() override;

	bool updateGeometry() override;
    virtual void updateClusterMatrices() override;
    void updateRenderItems() override;

    const Model::MeshState& getCauterizeMeshState(int index) const;

protected:
    std::unordered_set<int> _cauterizeBoneSet;
	QVector<Model::MeshState> _cauterizeMeshStates;
    bool _cauterizeBones { false };
};

#endif // hifi_CauterizedModel_h
