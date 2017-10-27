//
//  Created by Andrew Meadows 2016.01.17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CauterizedModel_h
#define hifi_CauterizedModel_h


#include "Model.h"

class CauterizedModel : public Model {
    Q_OBJECT

public:
    CauterizedModel(QObject* parent);
    virtual ~CauterizedModel();

    void flagAsCauterized() { _isCauterized = true; }
    bool getIsCauterized() const { return _isCauterized; }

    void setEnableCauterization(bool flag) { _enableCauterization = flag; }
    bool getEnableCauterization() const { return _enableCauterization; }

    const std::unordered_set<int>& getCauterizeBoneSet() const { return _cauterizeBoneSet; }
    void setCauterizeBoneSet(const std::unordered_set<int>& boneSet) { _cauterizeBoneSet = boneSet; }

    void deleteGeometry() override;
    bool updateGeometry() override;

    void createVisibleRenderItemSet() override;
    void createCollisionRenderItemSet() override;

    virtual void updateClusterMatrices() override;
    void updateRenderItems() override;

    const Model::MeshState& getCauterizeMeshState(int index) const;

protected:
    std::unordered_set<int> _cauterizeBoneSet;
    QVector<Model::MeshState> _cauterizeMeshStates;
    bool _isCauterized { false };
    bool _enableCauterization { false };
};

#endif // hifi_CauterizedModel_h
