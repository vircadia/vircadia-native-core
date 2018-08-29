//
//  AssociatedTraitValues.h
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 8/8/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssociatedTraitValues_h
#define hifi_AssociatedTraitValues_h

#include "AvatarTraits.h"

namespace AvatarTraits {
    template<typename T, T defaultValue>
    class AssociatedTraitValues {
    public:
        AssociatedTraitValues() : _simpleTypes(FirstInstancedTrait, defaultValue) {}

        void insert(TraitType type, T value) { _simpleTypes[type] = value; }
        void erase(TraitType type) { _simpleTypes[type] = defaultValue; }

        T& getInstanceValueRef(TraitType traitType, TraitInstanceID instanceID);
        void instanceInsert(TraitType traitType, TraitInstanceID instanceID, T value);

        struct InstanceIDValuePair {
            TraitInstanceID id;
            T value;

            InstanceIDValuePair(TraitInstanceID id, T value) : id(id), value(value) {};
        };

        using InstanceIDValuePairs = std::vector<InstanceIDValuePair>;

        InstanceIDValuePairs& getInstanceIDValuePairs(TraitType traitType);

        void instanceErase(TraitType traitType, TraitInstanceID instanceID);
        void eraseAllInstances(TraitType traitType);

        // will return defaultValue for instanced traits
        T operator[](TraitType traitType) const { return _simpleTypes[traitType]; }
        T& operator[](TraitType traitType) { return _simpleTypes[traitType]; }

        void reset() {
            std::fill(_simpleTypes.begin(), _simpleTypes.end(), defaultValue);
            _instancedTypes.clear();
        }

        typename std::vector<T>::const_iterator simpleCBegin() const { return _simpleTypes.cbegin(); }
        typename std::vector<T>::const_iterator simpleCEnd() const { return _simpleTypes.cend(); }

        typename std::vector<T>::iterator simpleBegin() { return _simpleTypes.begin(); }
        typename std::vector<T>::iterator simpleEnd() { return _simpleTypes.end(); }

        struct TraitWithInstances {
            TraitType traitType;
            InstanceIDValuePairs instances;

            TraitWithInstances(TraitType traitType) : traitType(traitType) {};
            TraitWithInstances(TraitType traitType, TraitInstanceID instanceID, T value) :
                traitType(traitType), instances({{ instanceID, value }}) {};
        };

        typename std::vector<TraitWithInstances>::const_iterator instancedCBegin() const { return _instancedTypes.cbegin(); }
        typename std::vector<TraitWithInstances>::const_iterator instancedCEnd() const { return _instancedTypes.cend(); }

        typename std::vector<TraitWithInstances>::iterator instancedBegin() { return _instancedTypes.begin(); }
        typename std::vector<TraitWithInstances>::iterator instancedEnd() { return _instancedTypes.end(); }

    private:
        std::vector<T> _simpleTypes;

        typename std::vector<TraitWithInstances>::iterator instancesForTrait(TraitType traitType) {
            return std::find_if(_instancedTypes.begin(), _instancedTypes.end(),
                                [traitType](TraitWithInstances& traitWithInstances){
                                    return traitWithInstances.traitType == traitType;
                                });
        }

        std::vector<TraitWithInstances> _instancedTypes;
    };

    template <typename T, T defaultValue>
    inline typename AssociatedTraitValues<T, defaultValue>::InstanceIDValuePairs&
    AssociatedTraitValues<T, defaultValue>::getInstanceIDValuePairs(TraitType traitType) {
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            return it->instances;
        } else {
            _instancedTypes.emplace_back(traitType);
            return _instancedTypes.back().instances;
        }
    }

    template <typename T, T defaultValue>
    inline T& AssociatedTraitValues<T, defaultValue>::getInstanceValueRef(TraitType traitType, TraitInstanceID instanceID) {
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            auto& instancesVector = it->instances;
            auto instanceIt = std::find_if(instancesVector.begin(), instancesVector.end(),
                                           [instanceID](InstanceIDValuePair& idValuePair){
                                               return idValuePair.id == instanceID;
                                           });
            if (instanceIt != instancesVector.end()) {
                return instanceIt->value;
            } else {
                instancesVector.emplace_back(instanceID, defaultValue);
                return instancesVector.back().value;
            }
        } else {
            _instancedTypes.emplace_back(traitType, instanceID, defaultValue);
            return _instancedTypes.back().instances.back().value;
        }
    }

    template <typename T, T defaultValue>
    inline void AssociatedTraitValues<T, defaultValue>::instanceInsert(TraitType traitType, TraitInstanceID instanceID, T value) {
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            auto& instancesVector = it->instances;
            auto instanceIt = std::find_if(instancesVector.begin(), instancesVector.end(),
                                           [instanceID](InstanceIDValuePair& idValuePair){
                                               return idValuePair.id == instanceID;
                                           });
            if (instanceIt != instancesVector.end()) {
                instanceIt->value = value;
            } else {
                instancesVector.emplace_back(instanceID, value);
            }
        } else {
            _instancedTypes.emplace_back(traitType, instanceID, value);
        }
    }

    template <typename T, T defaultValue>
    inline void AssociatedTraitValues<T, defaultValue>::instanceErase(TraitType traitType, TraitInstanceID instanceID) {
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            auto& instancesVector = it->instances;
            instancesVector.erase(std::remove_if(instancesVector.begin(),
                                                 instancesVector.end(),
                                                 [&instanceID](InstanceIDValuePair& idValuePair){
                                                     return idValuePair.id == instanceID;
                                                 }));
        }
    }

    using TraitVersions = AssociatedTraitValues<TraitVersion, DEFAULT_TRAIT_VERSION>;
};

#endif // hifi_AssociatedTraitValues_h
