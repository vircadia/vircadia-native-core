//
//  ModelReferential.h
//
//
//  Created by Clement on 7/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelReferential_h
#define hifi_ModelReferential_h

#include <Referential.h>

class ModelTree;
class Model;

class ModelReferential : public Referential {
public:
    ModelReferential(Referential* ref, ModelTree* tree, AvatarData* avatar);
    ModelReferential(uint32_t modelID, ModelTree* tree, AvatarData* avatar);
    virtual void update();
    
protected:
    virtual int packExtraData(unsigned char* destinationBuffer) const;
    virtual int unpackExtraData(const unsigned char* sourceBuffer, int size);
    
    uint32_t _modelID;
    ModelTree* _tree;
};

class JointReferential : public ModelReferential {
public:
    JointReferential(Referential* ref, ModelTree* tree, AvatarData* avatar);
    JointReferential(uint32_t jointIndex, uint32_t modelID, ModelTree* tree, AvatarData* avatar);
    virtual void update();
    
protected:
    const Model* getModel(const ModelItem* item);
    virtual int packExtraData(unsigned char* destinationBuffer) const;
    virtual int unpackExtraData(const unsigned char* sourceBuffer, int size);
    
    uint32_t _jointIndex;
};

#endif // hifi_ModelReferential_h