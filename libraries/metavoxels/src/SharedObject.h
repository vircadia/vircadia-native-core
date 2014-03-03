//
//  SharedObject.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 2/5/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__SharedObject__
#define __interface__SharedObject__

#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QWidget>
#include <QtDebug>

class QComboBox;

/// A QObject that may be shared over the network.
class SharedObject : public QObject {
    Q_OBJECT
    
public:

    Q_INVOKABLE SharedObject();

    int getID() { return _id; }

    int getHardReferenceCount() const { return _hardReferenceCount; }
    
    void incrementHardReferenceCount() { _hardReferenceCount++; }
    void decrementHardReferenceCount();

    void incrementSoftReferenceCount() { _softReferenceCount++; }
    void decrementSoftReferenceCount();
    
    /// Creates a new clone of this object.
    virtual SharedObject* clone() const;

    /// Tests this object for equality with another.    
    virtual bool equals(const SharedObject* other) const;

    // Dumps the contents of this object to the debug output.
    virtual void dump(QDebug debug = QDebug(QtDebugMsg)) const;

signals:
    
    /// Emitted when only soft reference counts remain.
    void allHardReferencesCleared(QObject* object);

private:
    
    int _id;
    int _hardReferenceCount;
    int _softReferenceCount;
    
    static int _lastID;
};

typedef void (SharedObject::*SharedObjectFn)();

/// A pointer to a shared object.
template<class T, SharedObjectFn Inc = &SharedObject::incrementHardReferenceCount,
    SharedObjectFn Dec = &SharedObject::decrementHardReferenceCount> class SharedObjectPointerTemplate {
public:
    
    SharedObjectPointerTemplate(T* data = NULL);
    SharedObjectPointerTemplate(const SharedObjectPointerTemplate<T, Inc, Dec>& other);
    template<class X, SharedObjectFn Inc2, SharedObjectFn Dec2> SharedObjectPointerTemplate(
        const SharedObjectPointerTemplate<X, Inc2, Dec2>& other);
    ~SharedObjectPointerTemplate();

    T* data() const { return _data; }
    
    /// "Detaches" this object, making a new copy if its reference count is greater than one.
    bool detach();

    void swap(SharedObjectPointerTemplate<T, Inc, Dec>& other) { qSwap(_data, other._data); }

    void reset();

    bool operator!() const { return !_data; }
    operator T*() const { return _data; }
    T& operator*() const { return *_data; }
    T* operator->() const { return _data; }
    
    template<class X> SharedObjectPointerTemplate<X, Inc, Dec> staticCast() const;
    
    SharedObjectPointerTemplate<T, Inc, Dec>& operator=(T* data);
    SharedObjectPointerTemplate<T, Inc, Dec>& operator=(const SharedObjectPointerTemplate<T, Inc, Dec>& other) {
        return *this = other.data(); }
    template<class X, SharedObjectFn Inc2, SharedObjectFn Dec2> SharedObjectPointerTemplate<T, Inc, Dec>& operator=(
        const SharedObjectPointerTemplate<X, Inc2, Dec2>& other) { return *this = other.data(); }

    bool operator==(T* data) const { return _data == data; }
    bool operator!=(T* data) const { return _data != data; }
    
    template<class X, SharedObjectFn Inc2, SharedObjectFn Dec2> bool operator==(
        const SharedObjectPointerTemplate<X, Inc2, Dec2>& other) const { return _data == other.data(); }
    template<class X, SharedObjectFn Inc2, SharedObjectFn Dec2> bool operator!=(
        const SharedObjectPointerTemplate<X, Inc2, Dec2>& other) const { return _data != other.data(); }
        
private:

    T* _data;
};

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> inline
        SharedObjectPointerTemplate<T, Inc, Dec>::SharedObjectPointerTemplate(T* data) : _data(data) {
    if (_data) {
        (_data->*Inc)();
    }
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> inline
    SharedObjectPointerTemplate<T, Inc, Dec>::SharedObjectPointerTemplate(
        const SharedObjectPointerTemplate<T, Inc, Dec>& other) : _data(other.data()) {
    if (_data) {
        (_data->*Inc)();
    }
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> template<class X, SharedObjectFn Inc2, SharedObjectFn Dec2> inline
    SharedObjectPointerTemplate<T, Inc, Dec>::SharedObjectPointerTemplate(
        const SharedObjectPointerTemplate<X, Inc2, Dec2>& other) : _data(other.data()) {
    if (_data) {
        (_data->*Inc)();
    }
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> inline
        SharedObjectPointerTemplate<T, Inc, Dec>::~SharedObjectPointerTemplate() {
    if (_data) {
        (_data->*Dec)();
    }
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> inline bool SharedObjectPointerTemplate<T, Inc, Dec>::detach() {
    if (_data && _data->getHardReferenceCount() > 1) {
        (_data->*Dec)();
        ((_data = _data->clone())->*Inc)();
        return true;
    }
    return false;
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> inline void SharedObjectPointerTemplate<T, Inc, Dec>::reset() {
    if (_data) {
        (_data->*Dec)();
    }
    _data = NULL;
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> template<class X> inline
        SharedObjectPointerTemplate<X, Inc, Dec> SharedObjectPointerTemplate<T, Inc, Dec>::staticCast() const {
    return SharedObjectPointerTemplate<X, Inc, Dec>(static_cast<X*>(_data));
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> inline
        SharedObjectPointerTemplate<T, Inc, Dec>& SharedObjectPointerTemplate<T, Inc, Dec>::operator=(T* data) {
    if (_data) {
        (_data->*Dec)();
    }
    if ((_data = data)) {
        (_data->*Inc)();
    }
    return *this;
}

template<class T, SharedObjectFn Inc, SharedObjectFn Dec> uint qHash(
        const SharedObjectPointerTemplate<T, Inc, Dec>& pointer, uint seed = 0) {
    return qHash(pointer.data(), seed);
}

typedef SharedObjectPointerTemplate<SharedObject> SharedObjectPointer;

Q_DECLARE_METATYPE(SharedObjectPointer)

typedef QSet<SharedObjectPointer> SharedObjectSet;

Q_DECLARE_METATYPE(SharedObjectSet)

typedef SharedObjectPointerTemplate<SharedObject, &SharedObject::incrementSoftReferenceCount,
    &SharedObject::decrementSoftReferenceCount> SoftSharedObjectPointer;

Q_DECLARE_METATYPE(SoftSharedObjectPointer)

/// Allows editing shared object instances.
class SharedObjectEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(SharedObjectPointer object READ getObject WRITE setObject USER true)

public:
    
    SharedObjectEditor(const QMetaObject* metaObject, bool nullable = true, QWidget* parent = NULL);

    const SharedObjectPointer& getObject() const { return _object; }

    /// "Detaches" the object pointer, copying it if anyone else is holding a reference.
    void detachObject();

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

#endif /* defined(__interface__SharedObject__) */
