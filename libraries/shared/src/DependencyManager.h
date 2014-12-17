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
    void customDeleter() {\
        QObject* thisObject = dynamic_cast<QObject*>(this);\
        if (thisObject) {\
            thisObject->deleteLater();\
        } else {\
            delete this;\
        }\
    }\
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
    static QSharedPointer<T> sharedPointer = QSharedPointer<T>(new T(), &T::customDeleter);
    return sharedPointer;
}

#endif // hifi_DependencyManager_h
