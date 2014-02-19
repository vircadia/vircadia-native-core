//
//  MetavoxelUtil.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QByteArray>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QItemEditorCreatorBase>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaType>
#include <QPushButton>
#include <QScriptEngine>
#include <QSettings>
#include <QVBoxLayout>
#include <QtDebug>

#include "MetavoxelUtil.h"
#include "ScriptCache.h"

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
}

DelegatingItemEditorFactory::DelegatingItemEditorFactory() :
    _parentFactory(QItemEditorFactory::defaultFactory()) {
    
    QItemEditorFactory::setDefaultFactory(this);
}

QWidget* DelegatingItemEditorFactory::createEditor(int userType, QWidget* parent) const {
    QWidget* editor = QItemEditorFactory::createEditor(userType, parent);
    return (editor == NULL) ? _parentFactory->createEditor(userType, parent) : editor;
}

QByteArray DelegatingItemEditorFactory::valuePropertyName(int userType) const {
    QByteArray propertyName = QItemEditorFactory::valuePropertyName(userType);
    return propertyName.isNull() ? _parentFactory->valuePropertyName(userType) : propertyName;
}

static QItemEditorFactory* getItemEditorFactory() {
    static QItemEditorFactory* factory = new DelegatingItemEditorFactory();
    return factory;
}

/// Because Windows doesn't necessarily have the staticMetaObject available when we want to create,
/// this class simply delays the value property name lookup until actually requested.
template<class T> class LazyItemEditorCreator : public QItemEditorCreatorBase {
public:
    
    virtual QWidget* createWidget(QWidget* parent) const { return new T(parent); }
    
    virtual QByteArray valuePropertyName() const;

protected:
    
    QByteArray _valuePropertyName;
};

template<class T> QByteArray LazyItemEditorCreator<T>::valuePropertyName() const {
    if (_valuePropertyName.isNull()) {
        const_cast<LazyItemEditorCreator<T>*>(this)->_valuePropertyName = T::staticMetaObject.userProperty().name();
    }
    return _valuePropertyName;    
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
static QItemEditorCreatorBase* parameterizedURLEditorCreator = createParameterizedURLEditorCreator();

QByteArray signal(const char* signature) {
    static QByteArray prototype = SIGNAL(dummyMethod());
    QByteArray signal = prototype;
    return signal.replace("dummyMethod()", signature);
}

Box::Box(const glm::vec3& minimum, const glm::vec3& maximum) :
    minimum(minimum), maximum(maximum) {
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

Vec3Editor::Vec3Editor(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(QMargins());
    setLayout(layout);
    
    layout->addWidget(_x = createComponentBox());
    layout->addWidget(_y = createComponentBox());
    layout->addWidget(_z = createComponentBox());
}

void Vec3Editor::setVector(const glm::vec3& vector) {
    _vector = vector;
    _x->setValue(vector.x);
    _y->setValue(vector.y);
    _z->setValue(vector.z);
}

void Vec3Editor::updateVector() {
    emit vectorChanged(_vector = glm::vec3(_x->value(), _y->value(), _z->value()));
}

QDoubleSpinBox* Vec3Editor::createComponentBox() {
    QDoubleSpinBox* box = new QDoubleSpinBox();
    box->setMinimum(-FLT_MAX);
    box->setMaximum(FLT_MAX);
    box->setMinimumWidth(50);
    connect(box, SIGNAL(valueChanged(double)), SLOT(updateVector()));
    return box;
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
