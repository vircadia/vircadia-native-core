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
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaType>
#include <QPushButton>
#include <QScriptEngine>
#include <QStandardItemEditorCreator>
#include <QVBoxLayout>
#include <QtDebug>

#include <HifiSockAddr.h>
#include <PacketHeaders.h>

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

static QItemEditorCreatorBase* createDoubleEditorCreator() {
    QItemEditorCreatorBase* creator = new QStandardItemEditorCreator<DoubleEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<double>(), creator);
    getItemEditorFactory()->registerEditor(qMetaTypeId<float>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createQColorEditorCreator() {
    QItemEditorCreatorBase* creator = new QStandardItemEditorCreator<QColorEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<QColor>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createVec3EditorCreator() {
    QItemEditorCreatorBase* creator = new QStandardItemEditorCreator<Vec3Editor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<glm::vec3>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createParameterizedURLEditorCreator() {
    QItemEditorCreatorBase* creator = new QStandardItemEditorCreator<ParameterizedURLEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<ParameterizedURL>(), creator);
    return creator;
}

static QItemEditorCreatorBase* doubleEditorCreator = createDoubleEditorCreator();
static QItemEditorCreatorBase* qColorEditorCreator = createQColorEditorCreator();
static QItemEditorCreatorBase* vec3EditorCreator = createVec3EditorCreator();
static QItemEditorCreatorBase* parameterizedURLEditorCreator = createParameterizedURLEditorCreator();

QUuid readSessionID(const QByteArray& data, const HifiSockAddr& sender, int& headerPlusIDSize) {
    // get the header size
    int headerSize = numBytesForPacketHeader(data);
    
    // read the session id
    const int UUID_BYTES = 16;
    headerPlusIDSize = headerSize + UUID_BYTES;
    if (data.size() < headerPlusIDSize) {
        qWarning() << "Metavoxel data too short [size=" << data.size() << ", sender=" << sender << "]\n";
        return QUuid();
    }
    return QUuid::fromRfc4122(QByteArray::fromRawData(data.constData() + headerSize, UUID_BYTES));
}

bool Box::contains(const Box& other) const {
    return other.minimum.x >= minimum.x && other.maximum.x <= maximum.x &&
        other.minimum.y >= minimum.y && other.maximum.y <= maximum.y &&
        other.minimum.z >= minimum.z && other.maximum.z <= maximum.z;
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
    box->setMaximumWidth(100);
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
    
    lineLayout->addWidget(_line = new QLineEdit(), 1);
    connect(_line, SIGNAL(textChanged(const QString&)), SLOT(updateURL()));
    
    QPushButton* refresh = new QPushButton("...");
    connect(refresh, SIGNAL(clicked(bool)), SLOT(updateParameters()));
    lineLayout->addWidget(refresh);
}

void ParameterizedURLEditor::setURL(const ParameterizedURL& url) {
    _url = url;
    _line->setText(url.getURL().toString());
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
    emit urlChanged(_url = ParameterizedURL(_line->text(), parameters));
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
                connect(widget, QByteArray(SIGNAL()).append(widgetProperty.notifySignal().methodSignature()),
                    SLOT(updateURL()));
            }
        }
    }
}
