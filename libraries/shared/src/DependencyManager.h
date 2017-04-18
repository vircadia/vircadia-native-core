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

#include <QDebug>
#include <QHash>
#include <QSharedPointer>
#include <QWeakPointer>

#include <functional>
#include <typeinfo>

#define SINGLETON_DEPENDENCY \
    friend class ::DependencyManager;

class Dependency {
public:
    typedef std::function<void(Dependency* pointer)> DeleterFunction;
    
protected:
    virtual ~Dependency() {}
    virtual void customDeleter() {
        _customDeleter(this);
    }
    
    void setCustomDeleter(DeleterFunction customDeleter) { _customDeleter = customDeleter; }
    DeleterFunction _customDeleter = [](Dependency* pointer) { delete pointer; };
    
    friend class DependencyManager;
};

// usage:
//     auto instance = DependencyManager::get<T>();
//     auto instance = DependencyManager::set<T>(Args... args);
//     DependencyManager::destroy<T>();
//     DependencyManager::registerInheritance<Base, Derived>();
class DependencyManager {
public:
    template<typename T>
    static QSharedPointer<T> get();
    
    template<typename T>
    static bool isSet();
    
    template<typename T, typename ...Args>
    static QSharedPointer<T> set(Args&&... args);

    template<typename T, typename I, typename ...Args>
    static QSharedPointer<T> set(Args&&... args);

    template<typename T>
    static void destroy();
    
    template<typename Base, typename Derived>
    static void registerInheritance();
    
private:
    static DependencyManager& manager();

    template<typename T>
    size_t getHashCode();
    
    QSharedPointer<Dependency>& safeGet(size_t hashCode);
    
    QHash<size_t, QSharedPointer<Dependency>> _instanceHash;
    QHash<size_t, size_t> _inheritanceHash;
};

template <typename T>
QSharedPointer<T> DependencyManager::get() {
    static size_t hashCode = manager().getHashCode<T>();
    static QWeakPointer<T> instance;
    
    if (instance.isNull()) {
        instance = qSharedPointerCast<T>(manager().safeGet(hashCode));
        
        if (instance.isNull()) {
            qWarning() << "DependencyManager::get(): No instance available for" << typeid(T).name();
        }
    }
    
    return instance.toStrongRef();
}

template <typename T>
bool DependencyManager::isSet() {
    static size_t hashCode = manager().getHashCode<T>();

    QSharedPointer<Dependency>& instance = manager().safeGet(hashCode);
    return !instance.isNull();
}

template <typename T, typename ...Args>
QSharedPointer<T> DependencyManager::set(Args&&... args) {
    static size_t hashCode = manager().getHashCode<T>();

    QSharedPointer<Dependency>& instance = manager().safeGet(hashCode);
    instance.clear(); // Clear instance before creation of new one to avoid edge cases
    QSharedPointer<T> newInstance(new T(args...), &T::customDeleter);
    QSharedPointer<Dependency> storedInstance = qSharedPointerCast<Dependency>(newInstance);
    instance.swap(storedInstance);

    return newInstance;
}

template <typename T, typename I, typename ...Args>
QSharedPointer<T> DependencyManager::set(Args&&... args) {
    static size_t hashCode = manager().getHashCode<T>();

    QSharedPointer<Dependency>& instance = manager().safeGet(hashCode);
    instance.clear(); // Clear instance before creation of new one to avoid edge cases
    QSharedPointer<T> newInstance(new I(args...), &I::customDeleter);
    QSharedPointer<Dependency> storedInstance = qSharedPointerCast<Dependency>(newInstance);
    instance.swap(storedInstance);

    return newInstance;
}

template <typename T>
void DependencyManager::destroy() {
    static size_t hashCode = manager().getHashCode<T>();
    manager().safeGet(hashCode).clear();
}

template<typename Base, typename Derived>
void DependencyManager::registerInheritance() {
    size_t baseHashCode = typeid(Base).hash_code();
    size_t derivedHashCode = typeid(Derived).hash_code();
    manager()._inheritanceHash.insert(baseHashCode, derivedHashCode);
}

template<typename T>
size_t DependencyManager::getHashCode() {
    size_t hashCode = typeid(T).hash_code();
    auto derivedHashCode = _inheritanceHash.find(hashCode);
    
    while (derivedHashCode != _inheritanceHash.end()) {
        hashCode = derivedHashCode.value();
        derivedHashCode = _inheritanceHash.find(hashCode);
    }
    
    return hashCode;
}

#endif // hifi_DependencyManager_h
