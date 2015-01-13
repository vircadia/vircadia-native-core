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
#include <QDebug>

#include <typeinfo>

#define SINGLETON_DEPENDENCY \
private:\
    void customDeleter() {\
        QObject* thisObject = dynamic_cast<QObject*>(this);\
        if (thisObject && thisObject->parent()) {\
            thisObject->deleteLater();\
        } else {\
            delete this;\
        }\
    }\
    friend class DependencyManager;

class QObject;

// usage:
//     T* instance = DependencyManager::get<T>();
//     T* instance = DependencyManager::set<T>(Args... args);
//     T* instance = DependencyManager::destroy<T>();
class DependencyManager {
public:
    template<typename T>
    static QSharedPointer<T> get();
    
    template<typename T, typename ...Args>
    static QSharedPointer<T> set(Args&&... args);
    
    template<typename T>
    static void destroy();
    
private:
    template<typename T>
    static QSharedPointer<T>& storage();
};

template <typename T>
QSharedPointer<T> DependencyManager::get() {
    return storage<T>();
}

template <typename T, typename ...Args>
QSharedPointer<T> DependencyManager::set(Args&&... args) {
    QSharedPointer<T> instance(new T(args...), &T::customDeleter);
    storage<T>().swap(instance);
    return storage<T>();
}

template <typename T>
void DependencyManager::destroy() {
    storage<T>().clear();
}

template<typename T>
QSharedPointer<T>& DependencyManager::storage() {
    static QSharedPointer<T> sharedPointer;
    return sharedPointer;
}

#endif // hifi_DependencyManager_h
