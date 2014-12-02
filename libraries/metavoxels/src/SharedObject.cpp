//
//  SharedObject.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 2/5/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QComboBox>
#include <QFormLayout>
#include <QItemEditorFactory>
#include <QMetaProperty>
#include <QVBoxLayout>
#include <QWriteLocker>

#include "Bitstream.h"
#include "MetavoxelUtil.h"
#include "SharedObject.h"

REGISTER_META_OBJECT(SharedObject)

SharedObject::SharedObject() :
    _id(_nextID.fetchAndAddOrdered(1)),
    _originID(_id),
    _remoteID(0),
    _remoteOriginID(0) {
    
    QWriteLocker locker(&_weakHashLock);
    _weakHash.insert(_id, this);
}

void SharedObject::setID(int id) {
    QWriteLocker locker(&_weakHashLock);
    _weakHash.remove(_id);
    _weakHash.insert(_id = id, this);
}

void SharedObject::incrementReferenceCount() {
    _referenceCount.ref();
}

void SharedObject::decrementReferenceCount() {
    if (!_referenceCount.deref()) {
        {
            QWriteLocker locker(&_weakHashLock);
            _weakHash.remove(_id);
        }
        delete this;
    }
}

SharedObject* SharedObject::clone(bool withID, SharedObject* target) const {
    // default behavior is to make a copy using the no-arg constructor and copy the stored properties
    const QMetaObject* metaObject = this->metaObject();
    if (!target) {
        target = static_cast<SharedObject*>(metaObject->newInstance());
    }
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored()) {
            if (property.userType() == qMetaTypeId<SharedObjectPointer>()) {
                SharedObject* value = property.read(this).value<SharedObjectPointer>().data();
                property.write(target, QVariant::fromValue(value ? value->clone(withID) : value));  
            } else {
                property.write(target, property.read(this));
            }
        }
    }
    foreach (const QByteArray& propertyName, dynamicPropertyNames()) {
        target->setProperty(propertyName, property(propertyName));
    }
    if (withID) {
        target->setOriginID(_originID);
    }
    return target;
}

bool SharedObject::equals(const SharedObject* other, bool sharedAncestry) const {
    if (!other) {
        return false;
    }
    if (other == this) {
        return true;
    }
    // default behavior is to compare the properties
    const QMetaObject* metaObject = this->metaObject();
    if (metaObject != other->metaObject() && !sharedAncestry) {
        return false;
    }
    // use the streamer, if we have one
    const ObjectStreamer* streamer = Bitstream::getObjectStreamer(metaObject);
    if (streamer) {
        if (!streamer->equal(this, other)) {
            return false;
        }
    } else {
        for (int i = 0; i < metaObject->propertyCount(); i++) {
            QMetaProperty property = metaObject->property(i);
            if (property.isStored() && property.read(this) != property.read(other)) {
                return false;
            }
        }
    }
    QList<QByteArray> dynamicPropertyNames = this->dynamicPropertyNames();
    if (dynamicPropertyNames.size() != other->dynamicPropertyNames().size()) {
        return false;
    }
    foreach (const QByteArray& propertyName, dynamicPropertyNames) {
        if (property(propertyName) != other->property(propertyName)) {
            return false;
        }
    }
    return true;
}

void SharedObject::dump(QDebug debug) const {
    debug << this;
    const QMetaObject* metaObject = this->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored()) {
            debug << property.name() << property.read(this);
        }
    }
    QList<QByteArray> dynamicPropertyNames = this->dynamicPropertyNames();
    foreach (const QByteArray& propertyName, dynamicPropertyNames) {
        debug << propertyName << property(propertyName);
    }
}

void SharedObject::writeExtra(Bitstream& out) const {
    // nothing by default
}

void SharedObject::readExtra(Bitstream& in) {
    // nothing by default
}

void SharedObject::writeExtraDelta(Bitstream& out, const SharedObject* reference) const {
    // nothing by default
}

void SharedObject::readExtraDelta(Bitstream& in, const SharedObject* reference) {
    // nothing by default
}

void SharedObject::writeExtraSubdivision(Bitstream& out) {
    // nothing by default
}

SharedObject* SharedObject::readExtraSubdivision(Bitstream& in) {
    return this;
}

QAtomicInt SharedObject::_nextID(1);
WeakSharedObjectHash SharedObject::_weakHash;
QReadWriteLock SharedObject::_weakHashLock;

void pruneWeakSharedObjectHash(WeakSharedObjectHash& hash) {
    for (WeakSharedObjectHash::iterator it = hash.begin(); it != hash.end(); ) {
        if (!it.value()) {
            it = hash.erase(it);
        } else {
            it++;
        }
    }
}

SharedObjectEditor::SharedObjectEditor(const QMetaObject* metaObject, bool nullable, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
    
    QFormLayout* form = new QFormLayout();    
    layout->addLayout(form);
    
    form->addRow("Type:", _type = new QComboBox());
    if (nullable) {
        _type->addItem("(none)");
    }
    foreach (const QMetaObject* metaObject, Bitstream::getMetaObjectSubClasses(metaObject)) {
        // add constructable subclasses
        if (metaObject->constructorCount() > 0) {
            _type->addItem(metaObject->className(), QVariant::fromValue(metaObject));
        }
    }
    connect(_type, SIGNAL(currentIndexChanged(int)), SLOT(updateType()));
    updateType();
}

void SharedObjectEditor::setObject(const SharedObjectPointer& object) {
    _object = object;
    const QMetaObject* metaObject = object ? object->metaObject() : NULL;
    int index = _type->findData(QVariant::fromValue(metaObject));
    if (index != -1) {
        // ensure that we call updateType to obtain the values
        if (_type->currentIndex() == index) {
            updateType();
        } else {
            _type->setCurrentIndex(index);
        }
    }
}

void SharedObjectEditor::detachObject() {
    SharedObject* oldObject = _object.data();
    if (!_object.detach()) {
        return;
    }
    oldObject->disconnect(this);
    const QMetaObject* metaObject = _object->metaObject();
    
    QFormLayout* form = static_cast<QFormLayout*>(layout()->itemAt(1));
    for (int i = 0; i < form->rowCount(); i++) {
        QWidget* widget = form->itemAt(i, QFormLayout::FieldRole)->widget();
        QMetaProperty property = metaObject->property(widget->property("propertyIndex").toInt());
        if (property.hasNotifySignal()) {
            connect(_object.data(), signal(property.notifySignal().methodSignature()), SLOT(updateProperty()));
        }
    }
}

const QMetaObject* getOwningAncestor(const QMetaObject* metaObject, int propertyIndex) {
    while (propertyIndex < metaObject->propertyOffset()) {
        metaObject = metaObject->superClass();
    }
    return metaObject;
}

void SharedObjectEditor::updateType() {
    // delete the existing rows
    if (layout()->count() > 1) {
        QFormLayout* form = static_cast<QFormLayout*>(layout()->takeAt(1));
        while (!form->isEmpty()) {
            QLayoutItem* item = form->takeAt(0);
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete form;
    }
    QObject* oldObject = static_cast<SharedObject*>(_object.data());
    const QMetaObject* oldMetaObject = NULL;
    if (oldObject) {
        oldMetaObject = oldObject->metaObject();
        oldObject->disconnect(this);
    }
    const QMetaObject* metaObject = _type->itemData(_type->currentIndex()).value<const QMetaObject*>();
    if (!metaObject) {
        _object.reset();
        emit objectChanged(_object);
        return;
    }
    QObject* newObject = metaObject->newInstance();
    
    QFormLayout* form = new QFormLayout();
    static_cast<QVBoxLayout*>(layout())->addLayout(form);
    for (int i = QObject::staticMetaObject.propertyCount(); i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (!property.isDesignable()) {
            continue;
        }
        if (oldMetaObject && i < oldMetaObject->propertyCount() &&
                getOwningAncestor(metaObject, i) == getOwningAncestor(oldMetaObject, i)) {
            // copy the state of the shared ancestry
            property.write(newObject, property.read(oldObject));
        }
        QWidget* widget = QItemEditorFactory::defaultFactory()->createEditor(property.userType(), NULL);
        if (widget) {
            widget->setProperty("propertyIndex", i);
            form->addRow(QByteArray(property.name()) + ':', widget);
            QByteArray valuePropertyName = QItemEditorFactory::defaultFactory()->valuePropertyName(property.userType());
            const QMetaObject* widgetMetaObject = widget->metaObject();
            QMetaProperty widgetProperty = widgetMetaObject->property(widgetMetaObject->indexOfProperty(valuePropertyName));
            widgetProperty.write(widget, property.read(newObject));
            if (widgetProperty.hasNotifySignal()) {
                connect(widget, signal(widgetProperty.notifySignal().methodSignature()), SLOT(propertyChanged()));
            }
            if (property.hasNotifySignal()) {
                widget->setProperty("notifySignalIndex", property.notifySignalIndex());
                connect(newObject, signal(property.notifySignal().methodSignature()), SLOT(updateProperty()));
            }
        }
    }
    emit objectChanged(_object = static_cast<SharedObject*>(newObject));
}

void SharedObjectEditor::propertyChanged() {
    QFormLayout* form = static_cast<QFormLayout*>(layout()->itemAt(1));
    for (int i = 0; i < form->rowCount(); i++) {
        QWidget* widget = form->itemAt(i, QFormLayout::FieldRole)->widget();
        if (widget != sender()) {
            continue;
        }
        detachObject();
        QObject* object = _object.data();
        QMetaProperty property = object->metaObject()->property(widget->property("propertyIndex").toInt());
        QByteArray valuePropertyName = QItemEditorFactory::defaultFactory()->valuePropertyName(property.userType());
        property.write(object, widget->property(valuePropertyName));
    }
    emit objectChanged(_object);
}

void SharedObjectEditor::updateProperty() {
    QFormLayout* form = static_cast<QFormLayout*>(layout()->itemAt(1));
    for (int i = 0; i < form->rowCount(); i++) {
        QWidget* widget = form->itemAt(i, QFormLayout::FieldRole)->widget();
        if (widget->property("notifySignalIndex").toInt() != senderSignalIndex()) {
            continue;
        }
        QMetaProperty property = _object->metaObject()->property(widget->property("propertyIndex").toInt());
        QByteArray valuePropertyName = QItemEditorFactory::defaultFactory()->valuePropertyName(property.userType());
        widget->setProperty(valuePropertyName, property.read(_object.data()));
    }
}
