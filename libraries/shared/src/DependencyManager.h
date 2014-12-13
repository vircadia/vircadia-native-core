//
//  DependencyManager.h
//
//
//  Created by Clément Brisset on 12/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DependencyManager_h
#define hifi_DependencyManager_h

#include <QHash>
#include <QString>

#include <typeinfo>

class DependencyManager {
public:
    template<typename T>
    static T* get();
    
    class Dependency {
        virtual void deleteInstance() = 0;
        friend DependencyManager;
    };
private:
    static DependencyManager& getInstance();
    DependencyManager();
    ~DependencyManager();
    
    typedef QHash<QString, Dependency*> InstanceHash;
    static InstanceHash& getInstanceHash() { return getInstance()._instanceHash; }
    InstanceHash _instanceHash;
};

template <typename T>
T* DependencyManager::get() {
    const QString& typeId = typeid(T).name();
    
    // Search the hash for global instance
    Dependency* instance = getInstanceHash().value(typeId, NULL);
    if (instance) {
        return dynamic_cast<T*>(instance);
    }

    // Found no instance in hash so we create one.
    T* newInstance = new T();
    getInstanceHash().insert(typeId, dynamic_cast<Dependency*>(newInstance));
    return newInstance;
}

#endif // hifi_DependencyManager_h