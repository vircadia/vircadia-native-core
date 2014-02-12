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
#include <QWidget>

class QComboBox;

/// A QObject that may be shared over the network.
class SharedObject : public QObject {
    Q_OBJECT
    
public:

    SharedObject();

    int getReferenceCount() const { return _referenceCount; }
    void incrementReferenceCount();
    void decrementReferenceCount();

    /// Creates a new clone of this object.
    virtual SharedObject* clone() const;

    /// Tests this object for equality with another.    
    virtual bool equals(const SharedObject* other) const;

signals:
    
    /// Emitted when the reference count drops to one.
    void referenceCountDroppedToOne();

private:
    
    int _referenceCount;
};

/// A pointer to a shared object.
template<class T> class SharedObjectPointerTemplate {
public:
    
    SharedObjectPointerTemplate(T* data = NULL);
    SharedObjectPointerTemplate(const SharedObjectPointerTemplate<T>& other);
    ~SharedObjectPointerTemplate();

    T* data() const { return _data; }
    
    void detach();

    void swap(SharedObjectPointerTemplate<T>& other) { qSwap(_data, other._data); }

    void reset();

    bool operator!() const { return !_data; }
    operator T*() const { return _data; }
    T& operator*() const { return *_data; }
    T* operator->() const { return _data; }
    
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

template<class T> inline void SharedObjectPointerTemplate<T>::detach() {
    if (_data && _data->getReferenceCount() > 1) {
        _data->decrementReferenceCount();
        (_data = _data->clone())->incrementReferenceCount();
    }
}

template<class T> inline void SharedObjectPointerTemplate<T>::reset() {
    if (_data) {
        _data->decrementReferenceCount();
    }
    _data = NULL;
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

typedef SharedObjectPointerTemplate<SharedObject> SharedObjectPointer;

Q_DECLARE_METATYPE(SharedObjectPointer)

/// Allows editing shared object instances.
class SharedObjectEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(SharedObjectPointer object MEMBER _object WRITE setObject USER true)

public:
    
    SharedObjectEditor(const QMetaObject* metaObject, QWidget* parent);

public slots:

    void setObject(const SharedObjectPointer& object);

private slots:

    void updateType();
    void propertyChanged();

private:
    
    QComboBox* _type;
    SharedObjectPointer _object;
};

#endif /* defined(__interface__SharedObject__) */
