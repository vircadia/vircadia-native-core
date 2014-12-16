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

#include <QSharedPointer>

#include <typeinfo>

#define SINGLETON_DEPENDENCY(T)\
public:\
    typedef QSharedPointer<T> SharedPointer;\
private:\
    void customDeleter() { delete this; }\
    friend class DependencyManager;

class QObject;

class DependencyManager {
public:
    // Only accessible method.
    // usage: T* instance = DependencyManager::get<T>();
    template<typename T>
    static QSharedPointer<T> get();
    
private:
    static DependencyManager& getInstance();
    DependencyManager() {}
    ~DependencyManager();
    
    static void noDelete(void*) {}
};

template <typename T>
QSharedPointer<T> DependencyManager::get() {
    static QSharedPointer<T> sharedPointer;
    static T* instance = new T();
    
    if (instance) {
        if (dynamic_cast<QObject*>(instance)) { // If this is a QOject, call deleteLater for destruction
            sharedPointer = QSharedPointer<T>(instance, &T::deleteLater);
        } else { // Otherwise use custom deleter to avoid issues between private destructor and QSharedPointer
            sharedPointer = QSharedPointer<T>(instance, &T::customDeleter);
        }
        instance = NULL;
    }
    return sharedPointer;
}

#endif // hifi_DependencyManager_h
