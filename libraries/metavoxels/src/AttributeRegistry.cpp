//
//  AttributeRegistry.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QItemEditorFactory>
#include <QMetaProperty>
#include <QPushButton>
#include <QScriptEngine>
#include <QVBoxLayout>

#include "AttributeRegistry.h"
#include "MetavoxelData.h"

REGISTER_META_OBJECT(QRgbAttribute)

AttributeRegistry* AttributeRegistry::getInstance() {
    static AttributeRegistry registry;
    return &registry;
}

AttributeRegistry::AttributeRegistry() :
    _guideAttribute(registerAttribute(new SharedObjectAttribute("guide", &MetavoxelGuide::staticMetaObject,
        PolymorphicDataPointer(new DefaultMetavoxelGuide())))),
    _colorAttribute(registerAttribute(new QRgbAttribute("color"))),
    _normalAttribute(registerAttribute(new QRgbAttribute("normal", qRgb(0, 127, 0)))) {
}

void AttributeRegistry::configureScriptEngine(QScriptEngine* engine) {
    QScriptValue registry = engine->newObject();
    registry.setProperty("colorAttribute", engine->newQObject(_colorAttribute.data()));
    registry.setProperty("normalAttribute", engine->newQObject(_normalAttribute.data()));
    registry.setProperty("getAttribute", engine->newFunction(getAttribute, 1));
    engine->globalObject().setProperty("AttributeRegistry", registry);
}

AttributePointer AttributeRegistry::registerAttribute(AttributePointer attribute) {
    AttributePointer& pointer = _attributes[attribute->getName()];
    if (!pointer) {
        pointer = attribute;
    }
    return pointer;
}

QScriptValue AttributeRegistry::getAttribute(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(getInstance()->getAttribute(context->argument(0).toString()).data(), QScriptEngine::QtOwnership,
        QScriptEngine::PreferExistingWrapperObject);
}

AttributeValue::AttributeValue(const AttributePointer& attribute) :
    _attribute(attribute), _value(attribute ? attribute->getDefaultValue() : NULL) {
}

AttributeValue::AttributeValue(const AttributePointer& attribute, void* value) :
    _attribute(attribute), _value(value) {
}

void* AttributeValue::copy() const {
    return _attribute->create(_value);
}

bool AttributeValue::isDefault() const {
    return !_attribute || _attribute->equal(_value, _attribute->getDefaultValue());
}

bool AttributeValue::operator==(const AttributeValue& other) const {
    return _attribute == other._attribute && (!_attribute || _attribute->equal(_value, other._value));
}

bool AttributeValue::operator==(void* other) const {
    return _attribute && _attribute->equal(_value, other);
}

OwnedAttributeValue::OwnedAttributeValue(const AttributePointer& attribute, void* value) :
    AttributeValue(attribute, value) {
}

OwnedAttributeValue::OwnedAttributeValue(const AttributePointer& attribute) :
    AttributeValue(attribute, attribute ? attribute->create() : NULL) {
}

OwnedAttributeValue::OwnedAttributeValue(const AttributeValue& other) :
    AttributeValue(other.getAttribute(), other.getAttribute() ? other.copy() : NULL) {
}

OwnedAttributeValue::~OwnedAttributeValue() {
    if (_attribute) {
        _attribute->destroy(_value);
    }
}

OwnedAttributeValue& OwnedAttributeValue::operator=(const AttributeValue& other) {
    if (_attribute) {
        _attribute->destroy(_value);
    }
    if ((_attribute = other.getAttribute())) {
        _value = other.copy();
    }
    return *this;
}

Attribute::Attribute(const QString& name) {
    setObjectName(name);
}

Attribute::~Attribute() {
}

QRgbAttribute::QRgbAttribute(const QString& name, QRgb defaultValue) :
    InlineAttribute<QRgb>(name, defaultValue) {
}
 
bool QRgbAttribute::merge(void*& parent, void* children[]) const {
    QRgb firstValue = decodeInline<QRgb>(children[0]);
    int totalRed = qRed(firstValue);
    int totalGreen = qGreen(firstValue);
    int totalBlue = qBlue(firstValue);
    int totalAlpha = qAlpha(firstValue);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        QRgb value = decodeInline<QRgb>(children[i]);
        totalRed += qRed(value);
        totalGreen += qGreen(value);
        totalBlue += qBlue(value);
        totalAlpha += qAlpha(value);
        allChildrenEqual &= (firstValue == value);
    }
    parent = encodeInline(qRgba(totalRed / MERGE_COUNT, totalGreen / MERGE_COUNT,
        totalBlue / MERGE_COUNT, totalAlpha / MERGE_COUNT));
    return allChildrenEqual;
} 

void* QRgbAttribute::createFromScript(const QScriptValue& value, QScriptEngine* engine) const {
    return encodeInline((QRgb)value.toUInt32());
}

void* QRgbAttribute::createFromVariant(const QVariant& value) const {
    switch (value.userType()) {
        case QMetaType::QColor:
            return encodeInline(value.value<QColor>().rgba());
        
        default:
            return encodeInline((QRgb)value.toUInt());
    } 
}

QWidget* QRgbAttribute::createEditor(QWidget* parent) const {
    QRgbEditor* editor = new QRgbEditor(parent);
    editor->setColor(QColor::fromRgba(_defaultValue));
    return editor;
}

QRgbEditor::QRgbEditor(QWidget* parent) : QWidget(parent) {
    setLayout(new QVBoxLayout());
    layout()->addWidget(_button = new QPushButton());
    connect(_button, SIGNAL(clicked()), SLOT(selectColor()));
}

void QRgbEditor::setColor(const QColor& color) {
    QString name = (_color = color).name();
    _button->setStyleSheet(QString("background: %1; color: %2").arg(name, QColor::fromRgb(~color.rgb()).name()));
    _button->setText(name);
}

void QRgbEditor::selectColor() {
    QColor color = QColorDialog::getColor(_color, this, QString(), QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        setColor(color);
    }
}

PolymorphicData::~PolymorphicData() {
}

template<> PolymorphicData* QExplicitlySharedDataPointer<PolymorphicData>::clone() {
    return d->clone();
}

PolymorphicAttribute::PolymorphicAttribute(const QString& name, const PolymorphicDataPointer& defaultValue) :
    InlineAttribute<PolymorphicDataPointer>(name, defaultValue) {
}

void* PolymorphicAttribute::createFromVariant(const QVariant& value) const {
    return create(encodeInline(value.value<PolymorphicDataPointer>()));
}

bool PolymorphicAttribute::merge(void*& parent, void* children[]) const {
    return false;
}

PolymorphicData* SharedObject::clone() const {
    // default behavior is to make a copy using the no-arg constructor and copy the stored properties
    const QMetaObject* metaObject = this->metaObject();
    QObject* newObject = metaObject->newInstance();
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored()) {
            property.write(newObject, property.read(this));
        }
    }
    foreach (const QByteArray& propertyName, dynamicPropertyNames()) {
        newObject->setProperty(propertyName, property(propertyName));
    }
    return static_cast<SharedObject*>(newObject);
}

SharedObjectAttribute::SharedObjectAttribute(const QString& name, const QMetaObject* metaObject,
        const PolymorphicDataPointer& defaultValue) :
    PolymorphicAttribute(name, defaultValue),
    _metaObject(metaObject) {
    
}

QWidget* SharedObjectAttribute::createEditor(QWidget* parent) const {
    SharedObjectEditor* editor = new SharedObjectEditor(_metaObject, parent);
    editor->setObject(_defaultValue);
    return editor;
}

SharedObjectEditor::SharedObjectEditor(const QMetaObject* metaObject, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);
    
    QFormLayout* form = new QFormLayout();    
    layout->addLayout(form);
    
    form->addRow("Type:", _type = new QComboBox());
    _type->addItem("(none)");
    foreach (const QMetaObject* metaObject, Bitstream::getMetaObjectSubClasses(metaObject)) {
        _type->addItem(metaObject->className(), QVariant::fromValue(metaObject));
    }
    connect(_type, SIGNAL(currentIndexChanged(int)), SLOT(updateType()));
}

void SharedObjectEditor::setObject(const PolymorphicDataPointer& object) {
    _object = object;
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
    const QMetaObject* metaObject = _type->itemData(_type->currentIndex()).value<const QMetaObject*>();
    if (metaObject == NULL) {
        return;
    }
    QFormLayout* form = new QFormLayout();
    static_cast<QVBoxLayout*>(layout())->addLayout(form);
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        QWidget* widget = QItemEditorFactory::defaultFactory()->createEditor(property.userType(), NULL);
        if (widget) {
            form->addRow(property.name(), widget);
        }
    }
}
