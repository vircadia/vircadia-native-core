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
class SharedObjectPointer {
public:
    
    SharedObjectPointer(SharedObject* data = NULL);
    SharedObjectPointer(const SharedObjectPointer& other);
    ~SharedObjectPointer();

    SharedObject* data() { return _data; }
    const SharedObject* data() const { return _data; }
    const SharedObject* constData() const { return _data; }

    void detach();

    void swap(SharedObjectPointer& other) { qSwap(_data, other._data); }

    void reset();

    operator SharedObject*() { return _data; }
    operator const SharedObject*() const { return _data; }

    bool operator!() const { return !_data; }

    bool operator!=(const SharedObjectPointer& other) const { return _data != other._data; }

    SharedObject& operator*() { return *_data; }
    const SharedObject& operator*() const { return *_data; }

    SharedObject* operator->() { return _data; }
    const SharedObject* operator->() const { return _data; }

    SharedObjectPointer& operator=(SharedObject* data);
    SharedObjectPointer& operator=(const SharedObjectPointer& other);

    bool operator==(const SharedObjectPointer& other) const { return _data == other._data; }

private:
    
    SharedObject* _data;
};

Q_DECLARE_METATYPE(SharedObjectPointer)

uint qHash(const SharedObjectPointer& pointer, uint seed = 0);

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
