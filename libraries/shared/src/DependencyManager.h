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

#include <QtGlobal>
#include <QDebug>
#include <QHash>
#include <QtCore/QSharedPointer>
#include <QWeakPointer>
#include <QMutex>

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

    template <typename T>
    static size_t typeHash() {
#ifdef Q_OS_ANDROID
        size_t hashCode = std::hash<std::string>{}( typeid(T).name() );
#else
        size_t hashCode = typeid(T).hash_code();
#endif
        return hashCode;
    }

    static void prepareToExit() { manager()._exiting = true; }

private:
    static DependencyManager& manager();

    template<typename T>
    size_t getHashCode() const;

    QSharedPointer<Dependency> safeGet(size_t hashCode) const;

    QHash<size_t, QSharedPointer<Dependency>> _instanceHash;
    QHash<size_t, size_t> _inheritanceHash;

    mutable QRecursiveMutex _instanceHashMutex;
    mutable QMutex _inheritanceHashMutex;

    bool _exiting { false };
};

template <typename T>
QSharedPointer<T> DependencyManager::get() {
    static size_t hashCode = manager().getHashCode<T>();
    static QWeakPointer<T> instance;

    if (instance.isNull()) {
        instance = qSharedPointerCast<T>(manager().safeGet(hashCode));

#ifndef QT_NO_DEBUG
        // debug builds...
        if (instance.isNull()) {
            qWarning() << "DependencyManager::get(): No instance available for" << typeid(T).name();
        }
#else
        // for non-debug builds, don't print "No instance available" during shutdown, because
        // the act of printing this often causes crashes (because the LogHandler has-been/is-being
        // deleted).
        if (!manager()._exiting && instance.isNull()) {
            qWarning() << "DependencyManager::get(): No instance available for" << typeid(T).name();
        }
#endif
    }

    return instance.toStrongRef();
}

template <typename T>
bool DependencyManager::isSet() {
    static size_t hashCode = manager().getHashCode<T>();

    QSharedPointer<Dependency> instance = manager().safeGet(hashCode);
    return !instance.isNull();
}

template <typename T, typename ...Args>
QSharedPointer<T> DependencyManager::set(Args&&... args) {
    static size_t hashCode = manager().getHashCode<T>();
    QMutexLocker lock(&manager()._instanceHashMutex);

    // clear the previous instance before constructing the new instance
    auto iter = manager()._instanceHash.find(hashCode);
    if (iter != manager()._instanceHash.end()) {
        iter.value().clear();
    }

    QSharedPointer<T> newInstance(new T(args...), &T::customDeleter);
    manager()._instanceHash.insert(hashCode, newInstance);

    return newInstance;
}

template <typename T, typename I, typename ...Args>
QSharedPointer<T> DependencyManager::set(Args&&... args) {
    static size_t hashCode = manager().getHashCode<T>();
    QMutexLocker lock(&manager()._instanceHashMutex);

    // clear the previous instance before constructing the new instance
    auto iter = manager()._instanceHash.find(hashCode);
    if (iter != manager()._instanceHash.end()) {
        iter.value().clear();
    }

    QSharedPointer<T> newInstance(new I(args...), &I::customDeleter);
    manager()._instanceHash.insert(hashCode, newInstance);

    return newInstance;
}

template <typename T>
void DependencyManager::destroy() {
    static size_t hashCode = manager().getHashCode<T>();

    QMutexLocker lock(&manager()._instanceHashMutex);
    QSharedPointer<Dependency> shared = manager()._instanceHash.take(hashCode);
    QWeakPointer<Dependency> weak = shared;
    shared.clear();

    // Check that the dependency was actually destroyed.  If it wasn't, it was improperly captured somewhere
    if (weak.lock()) {
        qWarning() << "DependencyManager::destroy():" << typeid(T).name() << "was not properly destroyed!";
    }
}

template<typename Base, typename Derived>
void DependencyManager::registerInheritance() {
    size_t baseHashCode = typeHash<Base>();
    size_t derivedHashCode = typeHash<Derived>();
    QMutexLocker lock(&manager()._inheritanceHashMutex);
    manager()._inheritanceHash.insert(baseHashCode, derivedHashCode);
}

template<typename T>
size_t DependencyManager::getHashCode() const {
    size_t hashCode = typeHash<T>();
    QMutexLocker lock(&_inheritanceHashMutex);
    auto derivedHashCode = _inheritanceHash.find(hashCode);

    while (derivedHashCode != _inheritanceHash.end()) {
        hashCode = derivedHashCode.value();
        derivedHashCode = _inheritanceHash.find(hashCode);
    }

    return hashCode;
}

#endif // hifi_DependencyManager_h
