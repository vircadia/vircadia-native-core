//
//  SharedObject.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 2/5/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SharedObject_h
#define hifi_SharedObject_h

#include <QAtomicInt>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QPointer>
#include <QReadWriteLock>
#include <QSet>
#include <QWidget>
#include <QtDebug>

class QComboBox;

class Bitstream;
class SharedObject;

typedef QHash<int, QPointer<SharedObject> > WeakSharedObjectHash;

/// A QObject that may be shared over the network.
class SharedObject : public QObject {
    Q_OBJECT
    
public:

    /// Returns the weak hash under which all local shared objects are registered.
    static const WeakSharedObjectHash& getWeakHash() { return _weakHash; }

    /// Returns a reference to the weak hash lock.
    static QReadWriteLock& getWeakHashLock() { return _weakHashLock; }
    
    Q_INVOKABLE SharedObject();

    /// Returns the unique local ID for this object.
    int getID() const { return _id; }

    void setID(int id);

    /// Returns the local origin ID for this object.
    int getOriginID() const { return _originID; }

    void setOriginID(int originID) { _originID = originID; }

    /// Returns the unique remote ID for this object, or zero if this is a local object.
    int getRemoteID() const { return _remoteID; }
    
    void setRemoteID(int remoteID) { _remoteID = remoteID; }

    /// Returns the remote origin ID for this object, or zero if this is a local object.
    int getRemoteOriginID() const { return _remoteOriginID; }    

    void setRemoteOriginID(int remoteOriginID) { _remoteOriginID = remoteOriginID; }

    int getReferenceCount() const { return _referenceCount.load(); }
    void incrementReferenceCount();
    void decrementReferenceCount();

    /// Creates a new clone of this object.
    /// \param withID if true, give the clone the same origin ID as this object
    /// \target if non-NULL, a target object to populate (as opposed to creating a new instance of this object's class)
    virtual SharedObject* clone(bool withID = false, SharedObject* target = NULL) const;

    /// Tests this object for equality with another.    
    /// \param sharedAncestry if true and the classes of the objects differ, compare their shared ancestry (assuming that
    /// this is an instance of a superclass of the other object's class) rather than simply returning false.
    virtual bool equals(const SharedObject* other, bool sharedAncestry = false) const;

    /// Dumps the contents of this object to the debug output.
    virtual void dump(QDebug debug = QDebug(QtDebugMsg)) const;

    /// Writes the non-property contents of this object to the specified stream.
    virtual void writeExtra(Bitstream& out) const;
    
    /// Reads the non-property contents of this object from the specified stream.
    virtual void readExtra(Bitstream& in);

    /// Writes the delta-encoded non-property contents of this object to the specified stream.
    virtual void writeExtraDelta(Bitstream& out, const SharedObject* reference) const;

    /// Reads the delta-encoded non-property contents of this object from the specified stream.
    virtual void readExtraDelta(Bitstream& in, const SharedObject* reference);

    /// Writes the subdivision of the non-property contents of this object to the specified stream.
    virtual void writeExtraSubdivision(Bitstream& out);
    
    /// Reads the subdivision of the non-property contents of this object from the specified stream.
    /// \return the modified object, or this if no modification was performed
    virtual SharedObject* readExtraSubdivision(Bitstream& in);

private:
    
    int _id;
    int _originID;
    int _remoteID;
    int _remoteOriginID;
    QAtomicInt _referenceCount;
    
    static QAtomicInt _nextID;
    static WeakSharedObjectHash _weakHash;
    static QReadWriteLock _weakHashLock;
};

/// Removes the null references from the supplied hash.
void pruneWeakSharedObjectHash(WeakSharedObjectHash& hash);

/// A pointer to a shared object.
template<class T> class SharedObjectPointerTemplate {
public:
    
    SharedObjectPointerTemplate(T* data = NULL);
    SharedObjectPointerTemplate(const SharedObjectPointerTemplate<T>& other);
    ~SharedObjectPointerTemplate();

    T* data() const { return _data; }
    
    /// "Detaches" this object, making a new copy if its reference count is greater than one.
    bool detach();

    void swap(SharedObjectPointerTemplate<T>& other) { qSwap(_data, other._data); }

    void reset();

    bool operator!() const { return !_data; }
    operator T*() const { return _data; }
    T& operator*() const { return *_data; }
    T* operator->() const { return _data; }
    
    template<class X> SharedObjectPointerTemplate<X> staticCast() const;
    
    SharedObjectPointerTemplate<T>& operator=(T* data);
    SharedObjectPointerTemplate<T>& operator=(const SharedObjectPointerTemplate<T>& other);

    bool operator==(const SharedObjectPointerTemplate<T>& other) const { return _data == other._data; }
    bool operator!=(const SharedObjectPointerTemplate<T>& other) const { return _data != other._data; }
    
private:

    T* _data;
};

template<class T> inline SharedObjectPointerTemplate<T>::SharedObjectPointerTemplate(T* data) : _data(data) {
    if (_data) {
        _data->incrementReferenceCount();
    }
}

template<class T> inline SharedObjectPointerTemplate<T>::SharedObjectPointerTemplate(const SharedObjectPointerTemplate<T>& other) :
    _data(other._data) {
    
    if (_data) {
        _data->incrementReferenceCount();
    }
}

template<class T> inline SharedObjectPointerTemplate<T>::~SharedObjectPointerTemplate() {
    if (_data) {
        _data->decrementReferenceCount();
    }
}

template<class T> inline bool SharedObjectPointerTemplate<T>::detach() {
    if (_data && _data->getReferenceCount() > 1) {
        _data->decrementReferenceCount();
        (_data = _data->clone())->incrementReferenceCount();
        return true;
    }
    return false;
}

template<class T> inline void SharedObjectPointerTemplate<T>::reset() {
    if (_data) {
        _data->decrementReferenceCount();
    }
    _data = NULL;
}

template<class T> template<class X> inline SharedObjectPointerTemplate<X> SharedObjectPointerTemplate<T>::staticCast() const {
    return SharedObjectPointerTemplate<X>(static_cast<X*>(_data));
}

template<class T> inline SharedObjectPointerTemplate<T>& SharedObjectPointerTemplate<T>::operator=(T* data) {
    if (_data) {
        _data->decrementReferenceCount();
    }
    if ((_data = data)) {
        _data->incrementReferenceCount();
    }
    return *this;
}

template<class T> inline SharedObjectPointerTemplate<T>& SharedObjectPointerTemplate<T>::operator=(
        const SharedObjectPointerTemplate<T>& other) {
    if (_data) {
        _data->decrementReferenceCount();
    }
    if ((_data = other._data)) {
        _data->incrementReferenceCount();
    }
    return *this;
}

template<class T> uint qHash(const SharedObjectPointerTemplate<T>& pointer, uint seed = 0) {
    return qHash(pointer.data(), seed);
}

template<class T, class X> bool equals(const SharedObjectPointerTemplate<T>& first,
        const SharedObjectPointerTemplate<X>& second) {
    return first ? first->equals(second) : !second;
}

typedef SharedObjectPointerTemplate<SharedObject> SharedObjectPointer;

Q_DECLARE_METATYPE(SharedObjectPointer)

typedef QSet<SharedObjectPointer> SharedObjectSet;

Q_DECLARE_METATYPE(SharedObjectSet)

/// Allows editing shared object instances.
class SharedObjectEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(SharedObjectPointer object READ getObject WRITE setObject NOTIFY objectChanged USER true)

public:
    
    SharedObjectEditor(const QMetaObject* metaObject, bool nullable = true, QWidget* parent = NULL);

    const SharedObjectPointer& getObject() const { return _object; }

    /// "Detaches" the object pointer, copying it if anyone else is holding a reference.
    void detachObject();

signals:

    void objectChanged(const SharedObjectPointer& object);

public slots:

    void setObject(const SharedObjectPointer& object);

private slots:

    void updateType();
    void propertyChanged();
    void updateProperty();

private:
    
    QComboBox* _type;
    SharedObjectPointer _object;
};

#endif // hifi_SharedObject_h
