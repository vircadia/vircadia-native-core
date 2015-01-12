//
//  MetavoxelUtil.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QByteArray>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaType>
#include <QPushButton>
#include <QScriptEngine>
#include <QSettings>
#include <QVBoxLayout>
#include <QtDebug>

#include <GLMHelpers.h>

#include "MetavoxelUtil.h"
#include "ScriptCache.h"
#include "StreamUtils.h"

static int scriptHashType = qRegisterMetaType<ScriptHash>();
static int parameterizedURLType = qRegisterMetaType<ParameterizedURL>();

REGISTER_SIMPLE_TYPE_STREAMER(ScriptHash)
REGISTER_SIMPLE_TYPE_STREAMER(ParameterizedURL)

class DelegatingItemEditorFactory : public QItemEditorFactory {
public:
    
    DelegatingItemEditorFactory();
    
    virtual QWidget* createEditor(int userType, QWidget* parent) const;
    virtual QByteArray valuePropertyName(int userType) const;
    
private:
    
    const QItemEditorFactory* _parentFactory;
};

class DoubleEditor : public QDoubleSpinBox {
public:
    
    DoubleEditor(QWidget* parent = NULL);
};

DoubleEditor::DoubleEditor(QWidget* parent) : QDoubleSpinBox(parent) {
    setMinimum(-FLT_MAX);
    setMaximum(FLT_MAX);
    setSingleStep(0.01);
}

DelegatingItemEditorFactory::DelegatingItemEditorFactory() :
    _parentFactory(QItemEditorFactory::defaultFactory()) {
    
    QItemEditorFactory::setDefaultFactory(this);
}

QWidget* DelegatingItemEditorFactory::createEditor(int userType, QWidget* parent) const {
    QWidget* editor = QItemEditorFactory::createEditor(userType, parent);
    return (!editor) ? _parentFactory->createEditor(userType, parent) : editor;
}

QByteArray DelegatingItemEditorFactory::valuePropertyName(int userType) const {
    QByteArray propertyName = QItemEditorFactory::valuePropertyName(userType);
    return propertyName.isNull() ? _parentFactory->valuePropertyName(userType) : propertyName;
}

QItemEditorFactory* getItemEditorFactory() {
    static QItemEditorFactory* factory = new DelegatingItemEditorFactory();
    return factory;
}

static QItemEditorCreatorBase* createDoubleEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<DoubleEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<double>(), creator);
    getItemEditorFactory()->registerEditor(qMetaTypeId<float>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createQMetaObjectEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<QMetaObjectEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<const QMetaObject*>(), creator);
    return creator;
} 

static QItemEditorCreatorBase* createQColorEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<QColorEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<QColor>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createQUrlEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<QUrlEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<QUrl>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createVec3EditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<Vec3Editor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<glm::vec3>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createQuatEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<QuatEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<glm::quat>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createParameterizedURLEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<ParameterizedURLEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<ParameterizedURL>(), creator);
    return creator;
}

static QItemEditorCreatorBase* doubleEditorCreator = createDoubleEditorCreator();
static QItemEditorCreatorBase* qMetaObjectEditorCreator = createQMetaObjectEditorCreator();
static QItemEditorCreatorBase* qColorEditorCreator = createQColorEditorCreator();
static QItemEditorCreatorBase* qUrlEditorCreator = createQUrlEditorCreator();
static QItemEditorCreatorBase* vec3EditorCreator = createVec3EditorCreator();
static QItemEditorCreatorBase* quatEditorCreator = createQuatEditorCreator();
static QItemEditorCreatorBase* parameterizedURLEditorCreator = createParameterizedURLEditorCreator();

QByteArray signal(const char* signature) {
    static QByteArray prototype = SIGNAL(dummyMethod());
    QByteArray signal = prototype;
    return signal.replace("dummyMethod()", signature);
}

Box::Box(const glm::vec3& minimum, const glm::vec3& maximum) :
    minimum(minimum), maximum(maximum) {
}

void Box::add(const Box& other) {
    minimum = glm::min(minimum, other.minimum);
    maximum = glm::max(maximum, other.maximum);
}

bool Box::contains(const glm::vec3& point) const {
    return point.x >= minimum.x && point.x <= maximum.x &&
        point.y >= minimum.y && point.y <= maximum.y &&
        point.z >= minimum.z && point.z <= maximum.z;
}

bool Box::contains(const Box& other) const {
    return other.minimum.x >= minimum.x && other.maximum.x <= maximum.x &&
        other.minimum.y >= minimum.y && other.maximum.y <= maximum.y &&
        other.minimum.z >= minimum.z && other.maximum.z <= maximum.z;
}

bool Box::intersects(const Box& other) const {
    return other.maximum.x >= minimum.x && other.minimum.x <= maximum.x &&
        other.maximum.y >= minimum.y && other.minimum.y <= maximum.y &&
        other.maximum.z >= minimum.z && other.minimum.z <= maximum.z;
}

Box Box::getIntersection(const Box& other) const {
    return Box(glm::max(minimum, other.minimum), glm::min(maximum, other.maximum));
}

bool Box::isEmpty() const {
    return minimum.x >= maximum.x || minimum.y >= maximum.y || minimum.z >= maximum.z;
}

const int X_MAXIMUM_FLAG = 1;
const int Y_MAXIMUM_FLAG = 2;
const int Z_MAXIMUM_FLAG = 4;

glm::vec3 Box::getVertex(int index) const {
    return glm::vec3(
        (index & X_MAXIMUM_FLAG) ? maximum.x : minimum.x,
        (index & Y_MAXIMUM_FLAG) ? maximum.y : minimum.y,
        (index & Z_MAXIMUM_FLAG) ? maximum.z : minimum.z); 
}

// finds the intersection between a ray and the facing plane on one axis
static bool findIntersection(float origin, float direction, float minimum, float maximum, float& distance) {
    if (direction > EPSILON) {
        distance = (minimum - origin) / direction;
        return true;
    } else if (direction < -EPSILON) {
        distance = (maximum - origin) / direction;
        return true;
    }
    return false;
}

// determines whether a value is within the extents
static bool isWithin(float value, float minimum, float maximum) {
    return value >= minimum && value <= maximum;
}

bool Box::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    // handle the trivial case where the box contains the origin
    if (contains(origin)) {
        distance = 0.0f;
        return true;
    }
    // check each axis
    float axisDistance;
    if ((findIntersection(origin.x, direction.x, minimum.x, maximum.x, axisDistance) && axisDistance >= 0 &&
            isWithin(origin.y + axisDistance*direction.y, minimum.y, maximum.y) &&
            isWithin(origin.z + axisDistance*direction.z, minimum.z, maximum.z))) {
        distance = axisDistance;
        return true;
    }
    if ((findIntersection(origin.y, direction.y, minimum.y, maximum.y, axisDistance) && axisDistance >= 0 &&
            isWithin(origin.x + axisDistance*direction.x, minimum.x, maximum.x) &&
            isWithin(origin.z + axisDistance*direction.z, minimum.z, maximum.z))) {
        distance = axisDistance;
        return true;
    }
    if ((findIntersection(origin.z, direction.z, minimum.z, maximum.z, axisDistance) && axisDistance >= 0 &&
            isWithin(origin.y + axisDistance*direction.y, minimum.y, maximum.y) &&
            isWithin(origin.x + axisDistance*direction.x, minimum.x, maximum.x))) {
        distance = axisDistance;
        return true;
    }
    return false;
}

Box operator*(const glm::mat4& matrix, const Box& box) {
    // start with the constant component
    Box newBox(glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]), glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]));

    // for each element, we choose the minimum or maximum based on the matrix sign
    if (matrix[0][0] >= 0.0f) {
        newBox.minimum.x += matrix[0][0] * box.minimum.x;
        newBox.maximum.x += matrix[0][0] * box.maximum.x;
    } else {
        newBox.minimum.x += matrix[0][0] * box.maximum.x;
        newBox.maximum.x += matrix[0][0] * box.minimum.x;
    }
    if (matrix[1][0] >= 0.0f) {
        newBox.minimum.x += matrix[1][0] * box.minimum.y;
        newBox.maximum.x += matrix[1][0] * box.maximum.y;
    } else {
        newBox.minimum.x += matrix[1][0] * box.maximum.y;
        newBox.maximum.x += matrix[1][0] * box.minimum.y;
    }
    if (matrix[2][0] >= 0.0f) {
        newBox.minimum.x += matrix[2][0] * box.minimum.z;
        newBox.maximum.x += matrix[2][0] * box.maximum.z;
    } else {
        newBox.minimum.x += matrix[2][0] * box.maximum.z;
        newBox.maximum.x += matrix[2][0] * box.minimum.z;
    }
    
    if (matrix[0][1] >= 0.0f) {
        newBox.minimum.y += matrix[0][1] * box.minimum.x;
        newBox.maximum.y += matrix[0][1] * box.maximum.x;
    } else {
        newBox.minimum.y += matrix[0][1] * box.maximum.x;
        newBox.maximum.y += matrix[0][1] * box.minimum.x;
    }
    if (matrix[1][1] >= 0.0f) {
        newBox.minimum.y += matrix[1][1] * box.minimum.y;
        newBox.maximum.y += matrix[1][1] * box.maximum.y;
    } else {
        newBox.minimum.y += matrix[1][1] * box.maximum.y;
        newBox.maximum.y += matrix[1][1] * box.minimum.y;
    }
    if (matrix[2][1] >= 0.0f) {
        newBox.minimum.y += matrix[2][1] * box.minimum.z;
        newBox.maximum.y += matrix[2][1] * box.maximum.z;
    } else {
        newBox.minimum.y += matrix[2][1] * box.maximum.z;
        newBox.maximum.y += matrix[2][1] * box.minimum.z;
    }
    
    if (matrix[0][2] >= 0.0f) {
        newBox.minimum.z += matrix[0][2] * box.minimum.x;
        newBox.maximum.z += matrix[0][2] * box.maximum.x;
    } else {
        newBox.minimum.z += matrix[0][2] * box.maximum.x;
        newBox.maximum.z += matrix[0][2] * box.minimum.x;
    }
    if (matrix[1][2] >= 0.0f) {
        newBox.minimum.z += matrix[1][2] * box.minimum.y;
        newBox.maximum.z += matrix[1][2] * box.maximum.y;
    } else {
        newBox.minimum.z += matrix[1][2] * box.maximum.y;
        newBox.maximum.z += matrix[1][2] * box.minimum.y;
    }
    if (matrix[2][2] >= 0.0f) {
        newBox.minimum.z += matrix[2][2] * box.minimum.z;
        newBox.maximum.z += matrix[2][2] * box.maximum.z;
    } else {
        newBox.minimum.z += matrix[2][2] * box.maximum.z;
        newBox.maximum.z += matrix[2][2] * box.minimum.z;
    }

    return newBox;
}

QDebug& operator<<(QDebug& dbg, const Box& box) {
    return dbg.nospace() << "{type='Box', minimum=" << box.minimum << ", maximum=" << box.maximum << "}";
}

AxisExtents::AxisExtents(const glm::vec3& first0, const glm::vec3& first1, const glm::vec3& first2, const glm::vec3& second) :
    axis(glm::cross(first2 - first1, first0 - first1)),
    minimum(glm::dot(first1, axis)),
    maximum(glm::dot(second, axis)) {
}

AxisExtents::AxisExtents(const glm::vec3& axis, float minimum, float maximum) :
    axis(axis),
    minimum(minimum),
    maximum(maximum) {
}

void Frustum::set(const glm::vec3& farTopLeft, const glm::vec3& farTopRight, const glm::vec3& farBottomLeft,
        const glm::vec3& farBottomRight, const glm::vec3& nearTopLeft, const glm::vec3& nearTopRight,
        const glm::vec3& nearBottomLeft, const glm::vec3& nearBottomRight) {

    _vertices[0] = farBottomLeft;
    _vertices[1] = farBottomRight;
    _vertices[2] = farTopLeft;
    _vertices[3] = farTopRight;
    _vertices[4] = nearBottomLeft;
    _vertices[5] = nearBottomRight;
    _vertices[6] = nearTopLeft;
    _vertices[7] = nearTopRight;
    
    // compute the bounds
    _bounds.minimum = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    _bounds.maximum = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < VERTEX_COUNT; i++) {
        _bounds.minimum = glm::min(_bounds.minimum, _vertices[i]);
        _bounds.maximum = glm::max(_bounds.maximum, _vertices[i]);
    }
    
    // compute the extents for each side
    _sideExtents[0] = AxisExtents(nearBottomLeft, nearTopLeft, nearTopRight, farBottomLeft);
    _sideExtents[1] = AxisExtents(nearBottomLeft, farBottomLeft, farTopLeft, farBottomRight);
    _sideExtents[2] = AxisExtents(nearBottomRight, nearTopRight, farTopRight, farBottomLeft);
    _sideExtents[3] = AxisExtents(nearBottomLeft, nearBottomRight, farBottomRight, farTopLeft);
    _sideExtents[4] = AxisExtents(nearTopLeft, farTopLeft, farTopRight, farBottomRight);
    
    // the other set of extents are derived from the cross products of the frustum and box edges
    glm::vec3 edges[] = { nearBottomRight - nearBottomLeft, nearTopLeft - nearBottomLeft, farBottomLeft - nearBottomLeft,
        farBottomRight - nearBottomRight,  farTopLeft - nearTopLeft, farTopRight - nearTopRight };
    const int AXIS_COUNT = 3;
    for (uint i = 0, extentIndex = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        for (int j = 0; j < AXIS_COUNT; j++) {
            glm::vec3 axis;
            axis[j] = 1.0f;
            glm::vec3 crossProduct = glm::cross(edges[i], axis);
            float minimum = FLT_MAX, maximum = -FLT_MAX;
            for (int k = 0; k < VERTEX_COUNT; k++) {
                float projection = glm::dot(crossProduct, _vertices[k]);
                minimum = glm::min(minimum, projection);
                maximum = glm::max(maximum, projection);
            }
            _crossProductExtents[extentIndex++] = AxisExtents(crossProduct, minimum, maximum);
        }
    }
}

Frustum::IntersectionType Frustum::getIntersectionType(const Box& box) const {
    // first check the bounds (equivalent to checking frustum vertices against box extents)
    if (!_bounds.intersects(box)) {
        return NO_INTERSECTION;
    }
    
    // check box vertices against side extents
    bool allInside = true;
    for (int i = 0; i < SIDE_EXTENT_COUNT; i++) {
        const AxisExtents& extents = _sideExtents[i];
        float firstProjection = glm::dot(box.getVertex(0), extents.axis);
        if (firstProjection < extents.minimum) {
            allInside = false;
            for (int j = 1; j < Box::VERTEX_COUNT; j++) {
                if (glm::dot(box.getVertex(j), extents.axis) >= extents.minimum) {
                    goto sideContinue;
                }
            }
            return NO_INTERSECTION;
            
        } else if (firstProjection > extents.maximum) {
            allInside = false;
            for (int j = 1; j < Box::VERTEX_COUNT; j++) {
                if (glm::dot(box.getVertex(j), extents.axis) <= extents.maximum) {
                    goto sideContinue;
                }
            }
            return NO_INTERSECTION;
        
        } else if (allInside) {
            for (int j = 1; j < Box::VERTEX_COUNT; j++) {
                float projection = glm::dot(box.getVertex(j), extents.axis);
                if (projection < extents.minimum || projection > extents.maximum) {
                    allInside = false;
                    goto sideContinue;
                }
            }
        }
        sideContinue: ;
    }
    if (allInside) {
        return CONTAINS_INTERSECTION;
    }
    
    // check box vertices against cross product extents
    for (int i = 0; i < CROSS_PRODUCT_EXTENT_COUNT; i++) {
        const AxisExtents& extents = _crossProductExtents[i];
        float firstProjection = glm::dot(box.getVertex(0), extents.axis);
        if (firstProjection < extents.minimum) {
            for (int j = 1; j < Box::VERTEX_COUNT; j++) {
                if (glm::dot(box.getVertex(j), extents.axis) >= extents.minimum) {
                    goto crossProductContinue;
                }
            }
            return NO_INTERSECTION;
            
        } else if (firstProjection > extents.maximum) {
            for (int j = 1; j < Box::VERTEX_COUNT; j++) {
                if (glm::dot(box.getVertex(j), extents.axis) <= extents.maximum) {
                    goto crossProductContinue;
                }
            }
            return NO_INTERSECTION;
        }
        crossProductContinue: ;
    }
    
    return PARTIAL_INTERSECTION;
}

QMetaObjectEditor::QMetaObjectEditor(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(QMargins());
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
    layout->addWidget(_box = new QComboBox());
    connect(_box, SIGNAL(currentIndexChanged(int)), SLOT(updateMetaObject()));
    
    foreach (const QMetaObject* metaObject, Bitstream::getMetaObjectSubClasses(&SharedObject::staticMetaObject)) {
        _box->addItem(metaObject->className(), QVariant::fromValue(metaObject));
    }
}

void QMetaObjectEditor::setMetaObject(const QMetaObject* metaObject) {
    _metaObject = metaObject;
    _box->setCurrentIndex(_metaObject ? _box->findText(_metaObject->className()) : -1);
}

void QMetaObjectEditor::updateMetaObject() {
    int index = _box->currentIndex();
    emit metaObjectChanged(_metaObject = (index == -1) ? NULL : _box->itemData(index).value<const QMetaObject*>());
}

QColorEditor::QColorEditor(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(QMargins());
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
    layout->addWidget(_button = new QPushButton());
    connect(_button, SIGNAL(clicked()), SLOT(selectColor()));
    setColor(QColor());
}

void QColorEditor::setColor(const QColor& color) {
    QString name = (_color = color).name();
    _button->setStyleSheet(QString("background: %1; color: %2").arg(name, QColor::fromRgb(~color.rgb()).name()));
    _button->setText(name);
}

void QColorEditor::selectColor() {
    QColor color = QColorDialog::getColor(_color, this, QString(), QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        setColor(color);
        emit colorChanged(color);
    }
}

QUrlEditor::QUrlEditor(QWidget* parent) :
    QComboBox(parent) {
    
    setEditable(true);
    setInsertPolicy(InsertAtTop);
    
    // populate initial URL list from settings
    addItems(QSettings().value("editorURLs").toStringList());
    
    connect(this, SIGNAL(activated(const QString&)), SLOT(updateURL(const QString&)));
    connect(model(), SIGNAL(rowsInserted(const QModelIndex&,int,int)), SLOT(updateSettings()));
}

void QUrlEditor::setURL(const QUrl& url) {
    setCurrentText((_url = url).toString());
}

void QUrlEditor::updateURL(const QString& text) {
    emit urlChanged(_url = text);
}

void QUrlEditor::updateSettings() {
    QStringList urls;
    const int MAX_STORED_URLS = 10;
    for (int i = 0, size = qMin(MAX_STORED_URLS, count()); i < size; i++) {
        urls.append(itemText(i));
    }
    QSettings().setValue("editorURLs", urls);
}

BaseVec3Editor::BaseVec3Editor(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(QMargins());
    setLayout(layout);
    
    layout->addWidget(_x = createComponentBox());
    layout->addWidget(_y = createComponentBox());
    layout->addWidget(_z = createComponentBox());
}

void BaseVec3Editor::setSingleStep(double singleStep) {
    _x->setSingleStep(singleStep);
    _y->setSingleStep(singleStep);
    _z->setSingleStep(singleStep);
}

double BaseVec3Editor::getSingleStep() const {
    return _x->singleStep();
}

QDoubleSpinBox* BaseVec3Editor::createComponentBox() {
    QDoubleSpinBox* box = new QDoubleSpinBox();
    box->setMinimum(-FLT_MAX);
    box->setMaximum(FLT_MAX);
    box->setMinimumWidth(50);
    connect(box, SIGNAL(valueChanged(double)), SLOT(updateValue()));
    return box;
}

Vec3Editor::Vec3Editor(QWidget* parent) : BaseVec3Editor(parent) {
    setSingleStep(0.01);
}

static void setComponentValue(QDoubleSpinBox* box, double value) {
    box->blockSignals(true);
    box->setValue(value);
    box->blockSignals(false);
}

void Vec3Editor::setValue(const glm::vec3& value) {
    _value = value;
    setComponentValue(_x, value.x);
    setComponentValue(_y, value.y);
    setComponentValue(_z, value.z);
}

void Vec3Editor::updateValue() {
    emit valueChanged(_value = glm::vec3(_x->value(), _y->value(), _z->value()));
}

QuatEditor::QuatEditor(QWidget* parent) : BaseVec3Editor(parent) {
    _x->setRange(-179.0, 180.0);
    _y->setRange(-179.0, 180.0);
    _z->setRange(-179.0, 180.0);
    
    _x->setWrapping(true);
    _y->setWrapping(true);
    _z->setWrapping(true);
}

void QuatEditor::setValue(const glm::quat& value) {
    if (_value != value) {
        glm::vec3 eulers = glm::degrees(safeEulerAngles(_value = value));
        setComponentValue(_x, eulers.x);
        setComponentValue(_y, eulers.y);
        setComponentValue(_z, eulers.z);
    }
}

void QuatEditor::updateValue() {
    glm::quat value(glm::radians(glm::vec3(_x->value(), _y->value(), _z->value())));
    if (_value != value) {
        emit valueChanged(_value = value);
    }
}

ParameterizedURL::ParameterizedURL(const QUrl& url, const ScriptHash& parameters) :
    _url(url),
    _parameters(parameters) {
}

bool ParameterizedURL::operator==(const ParameterizedURL& other) const {
    return _url == other._url && _parameters == other._parameters;
}

bool ParameterizedURL::operator!=(const ParameterizedURL& other) const {
    return _url != other._url || _parameters != other._parameters;
}

uint qHash(const ParameterizedURL& url, uint seed) {
    // just hash on the URL, for now
    return qHash(url.getURL(), seed);
}

Bitstream& operator<<(Bitstream& out, const ParameterizedURL& url) {
    out << url.getURL();
    out << url.getParameters();
    return out;
}

Bitstream& operator>>(Bitstream& in, ParameterizedURL& url) {
    QUrl qurl;
    in >> qurl;
    ScriptHash parameters;
    in >> parameters;
    url = ParameterizedURL(qurl, parameters);
    return in;
}

ParameterizedURLEditor::ParameterizedURLEditor(QWidget* parent) :
    QWidget(parent) {

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(QMargins());
    setLayout(layout);
    
    QWidget* lineContainer = new QWidget();
    layout->addWidget(lineContainer);
    
    QHBoxLayout* lineLayout = new QHBoxLayout();
    lineContainer->setLayout(lineLayout);
    lineLayout->setContentsMargins(QMargins());
    
    lineLayout->addWidget(&_urlEditor, 1);
    connect(&_urlEditor, SIGNAL(urlChanged(const QUrl&)), SLOT(updateURL()));
    connect(&_urlEditor, SIGNAL(urlChanged(const QUrl&)), SLOT(updateParameters()));
    
    QPushButton* refresh = new QPushButton("...");
    connect(refresh, SIGNAL(clicked(bool)), SLOT(updateParameters()));
    lineLayout->addWidget(refresh);
}

void ParameterizedURLEditor::setURL(const ParameterizedURL& url) {
    _urlEditor.setURL((_url = url).getURL());
    updateParameters();
}

void ParameterizedURLEditor::updateURL() {
    ScriptHash parameters;
    if (layout()->count() > 1) {
        QFormLayout* form = static_cast<QFormLayout*>(layout()->itemAt(1));
        for (int i = 0; i < form->rowCount(); i++) {
            QWidget* widget = form->itemAt(i, QFormLayout::FieldRole)->widget();
            QByteArray valuePropertyName = widget->property("valuePropertyName").toByteArray();
            const QMetaObject* widgetMetaObject = widget->metaObject();
            QMetaProperty widgetProperty = widgetMetaObject->property(widgetMetaObject->indexOfProperty(valuePropertyName));
            parameters.insert(ScriptCache::getInstance()->getEngine()->toStringHandle(
                widget->property("parameterName").toString()), widgetProperty.read(widget));
        }
    }
    emit urlChanged(_url = ParameterizedURL(_urlEditor.getURL(), parameters));
    if (_program) {
        _program->disconnect(this);
    }
}

void ParameterizedURLEditor::updateParameters() {    
    if (_program) {
        _program->disconnect(this);
    }
    _program = ScriptCache::getInstance()->getProgram(_url.getURL());
    if (_program->isLoaded()) {
        continueUpdatingParameters();
    } else {
        connect(_program.data(), SIGNAL(loaded()), SLOT(continueUpdatingParameters()));
    }
}

void ParameterizedURLEditor::continueUpdatingParameters() {
    QVBoxLayout* layout = static_cast<QVBoxLayout*>(this->layout());
    if (layout->count() > 1) {
        QFormLayout* form = static_cast<QFormLayout*>(layout->takeAt(1));
        for (int i = form->count() - 1; i >= 0; i--) {
            QLayoutItem* item = form->takeAt(i);
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete form;
    }
    QSharedPointer<NetworkValue> value = ScriptCache::getInstance()->getValue(_url.getURL());
    const QList<ParameterInfo>& parameters = static_cast<RootNetworkValue*>(value.data())->getParameterInfo();
    if (parameters.isEmpty()) {
        return;
    }
    QFormLayout* form = new QFormLayout();
    layout->addLayout(form);
    foreach (const ParameterInfo& parameter, parameters) {
        QWidget* widget = QItemEditorFactory::defaultFactory()->createEditor(parameter.type, NULL);
        if (widget) {
            form->addRow(parameter.name.toString() + ":", widget);
            QByteArray valuePropertyName = QItemEditorFactory::defaultFactory()->valuePropertyName(parameter.type);
            widget->setProperty("parameterName", parameter.name.toString());
            widget->setProperty("valuePropertyName", valuePropertyName);
            const QMetaObject* widgetMetaObject = widget->metaObject();
            QMetaProperty widgetProperty = widgetMetaObject->property(widgetMetaObject->indexOfProperty(valuePropertyName));
            widgetProperty.write(widget, _url.getParameters().value(parameter.name));
            if (widgetProperty.hasNotifySignal()) {
                connect(widget, signal(widgetProperty.notifySignal().methodSignature()), SLOT(updateURL()));
            }
        }
    }
}
