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

// This templated class is admittedly fairly confusing to look at. It is used
// to hold some associated value of type T for both simple (non-instanced) and instanced traits.
// Most of the complexity comes from the fact that simple and instanced trait types are
// handled differently. For each simple trait type there can be a value T, but for
// each instance of each instanced trait
// (keyed by a TraitInstanceID, which at the time of this writing is a UUID) there can be a value T.
// There are separate methods in most cases for simple traits and instanced traits
// because of this different behaviour. This class is not used to hold the values
// of the traits themselves, but instead an associated value like the latest version
// of each trait (see TraitVersions) or a state associated with each trait (like added/changed/deleted).

namespace AvatarTraits {
    template<typename T, T defaultValue>
    class AssociatedTraitValues {
        using SimpleTypesArray = std::array<T, NUM_SIMPLE_TRAITS>;
    public:
        // constructor that pre-fills _simpleTypes with the default value specified by the template
        AssociatedTraitValues() { std::fill(_simpleTypes.begin(), _simpleTypes.end(), defaultValue); }

        /// inserts the given value for the given simple trait type
        void insert(TraitType type, T value) { _simpleTypes[type] = value; }
        /// resets the simple trait type value to the default
        void erase(TraitType type) { _simpleTypes[type] = defaultValue; }

        /// returns a reference to the value for a given instance for a given instanced trait type
        T& getInstanceValueRef(TraitType traitType, TraitInstanceID instanceID);

        /// inserts the passed value for the given instance for the given instanced trait type
        void instanceInsert(TraitType traitType, TraitInstanceID instanceID, T value);

        struct InstanceIDValuePair {
            TraitInstanceID id;
            T value;

            InstanceIDValuePair(TraitInstanceID id, T value) : id(id), value(value) {};
        };

        using InstanceIDValuePairs = std::vector<InstanceIDValuePair>;
        
        /// returns a vector of InstanceIDValuePair objects for the given instanced trait type
        InstanceIDValuePairs& getInstanceIDValuePairs(TraitType traitType);

        /// erases the a given instance for a given instanced trait type
        void instanceErase(TraitType traitType, TraitInstanceID instanceID);
        /// erases the value for all instances for a given instanced trait type
        void eraseAllInstances(TraitType traitType);

        /// value getters for simple trait types, will be default value if value has been erased or not set
        T operator[](TraitType traitType) const { return _simpleTypes[traitType]; }
        T& operator[](TraitType traitType) { return _simpleTypes[traitType]; }

        /// resets all simple trait types to the default value and erases all values for instanced trait types
        void reset() {
            std::fill(_simpleTypes.begin(), _simpleTypes.end(), defaultValue);
            _instancedTypes.clear();
        }

        /// const iterators for the vector of simple type values
        typename SimpleTypesArray::const_iterator simpleCBegin() const { return _simpleTypes.cbegin(); }
        typename SimpleTypesArray::const_iterator simpleCEnd() const { return _simpleTypes.cend(); }

        /// non-const iterators for the vector of simple type values
        typename SimpleTypesArray::iterator simpleBegin() { return _simpleTypes.begin(); }
        typename SimpleTypesArray::iterator simpleEnd() { return _simpleTypes.end(); }

        struct TraitWithInstances {
            TraitType traitType;
            InstanceIDValuePairs instances;

            TraitWithInstances(TraitType traitType) : traitType(traitType) {};
            TraitWithInstances(TraitType traitType, TraitInstanceID instanceID, T value) :
                traitType(traitType), instances({{ instanceID, value }}) {};
        };

        /// const iterators for the vector of TraitWithInstances objects
        typename std::vector<TraitWithInstances>::const_iterator instancedCBegin() const { return _instancedTypes.cbegin(); }
        typename std::vector<TraitWithInstances>::const_iterator instancedCEnd() const { return _instancedTypes.cend(); }

        /// non-const iterators for the vector of TraitWithInstances objects
        typename std::vector<TraitWithInstances>::iterator instancedBegin() { return _instancedTypes.begin(); }
        typename std::vector<TraitWithInstances>::iterator instancedEnd() { return _instancedTypes.end(); }

    private:
        SimpleTypesArray _simpleTypes;

        /// return the iterator to the matching TraitWithInstances object for a given instanced trait type
        typename std::vector<TraitWithInstances>::iterator instancesForTrait(TraitType traitType) {
            return std::find_if(_instancedTypes.begin(), _instancedTypes.end(),
                                [traitType](TraitWithInstances& traitWithInstances){
                                    return traitWithInstances.traitType == traitType;
                                });
        }

        std::vector<TraitWithInstances> _instancedTypes;
    };

    /// returns a reference to the InstanceIDValuePairs object for a given instanced trait type
    template <typename T, T defaultValue>
    inline typename AssociatedTraitValues<T, defaultValue>::InstanceIDValuePairs&
    AssociatedTraitValues<T, defaultValue>::getInstanceIDValuePairs(TraitType traitType) {
        // first check if we already have some values for instances of this trait type
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            return it->instances;
        } else {
            // if we didn't have any values for instances of the instanced trait type
            // add an empty InstanceIDValuePairs object first and then return the reference to it
            _instancedTypes.emplace_back(traitType);
            return _instancedTypes.back().instances;
        }
    }

    // returns a reference to value for the given instance of the given instanced trait type
    template <typename T, T defaultValue>
    inline T& AssociatedTraitValues<T, defaultValue>::getInstanceValueRef(TraitType traitType, TraitInstanceID instanceID) {
        // first check if we already have some values for instances of this trait type
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            // grab the matching vector of instances
            auto& instancesVector = it->instances;

            // check if we have a value for this specific instance ID
            auto instanceIt = std::find_if(instancesVector.begin(), instancesVector.end(),
                                           [instanceID](InstanceIDValuePair& idValuePair){
                                               return idValuePair.id == instanceID;
                                           });
            if (instanceIt != instancesVector.end()) {
                return instanceIt->value;
            } else {
                // no value for this specific instance ID, insert the default value and return it
                instancesVector.emplace_back(instanceID, defaultValue);
                return instancesVector.back().value;
            }
        } else {
            // no values for any instances of this trait type
            // insert the default value for the specific instance for the instanced trait type
            _instancedTypes.emplace_back(traitType, instanceID, defaultValue);
            return _instancedTypes.back().instances.back().value;
        }
    }

    /// inserts the passed value for the specific instance of the given instanced trait type
    template <typename T, T defaultValue>
    inline void AssociatedTraitValues<T, defaultValue>::instanceInsert(TraitType traitType, TraitInstanceID instanceID, T value) {
        // first check if we already have some instances for this trait type
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            // found some instances for the instanced trait type, check if our specific instance is one of them
            auto& instancesVector = it->instances;
            auto instanceIt = std::find_if(instancesVector.begin(), instancesVector.end(),
                                           [instanceID](InstanceIDValuePair& idValuePair){
                                               return idValuePair.id == instanceID;
                                           });
            if (instanceIt != instancesVector.end()) {
                // the instance already existed, update the value
                instanceIt->value = value;
            } else {
                // the instance was not present, emplace the new value
                instancesVector.emplace_back(instanceID, value);
            }
        } else {
            // there were no existing instances for the given trait type
            // setup the container for instances and insert the passed value for this instance ID
            _instancedTypes.emplace_back(traitType, instanceID, value);
        }
    }

    /// erases the value for a specific instance of the given instanced trait type
    template <typename T, T defaultValue>
    inline void AssociatedTraitValues<T, defaultValue>::instanceErase(TraitType traitType, TraitInstanceID instanceID) {
        // check if we have any instances at all for this instanced trait type
        auto it = instancesForTrait(traitType);

        if (it != _instancedTypes.end()) {
            // we have some instances, erase the value for the passed instance ID if it is present
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
