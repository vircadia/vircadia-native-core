//
//  MetavoxelUtil.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QByteArray>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QMetaType>
#include <QPushButton>
#include <QStandardItemEditorCreator>
#include <QVBoxLayout>
#include <QtDebug>

#include <HifiSockAddr.h>
#include <PacketHeaders.h>

#include "MetavoxelUtil.h"

static int parameterizedURLType = qRegisterMetaType<ParameterizedURL>();

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

static QItemEditorCreatorBase* createParameterizedURLEditorCreator() {
    QItemEditorCreatorBase* creator = new QStandardItemEditorCreator<ParameterizedURLEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<ParameterizedURL>(), creator);
    return creator;
}

static QItemEditorCreatorBase* doubleEditorCreator = createDoubleEditorCreator();
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

ParameterizedURL::ParameterizedURL(const QUrl& url, const QVariantHash& parameters) :
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
    QVariantHash parameters;
    in >> parameters;
    url = ParameterizedURL(qurl, parameters);
    return in;
}

ParameterizedURLEditor::ParameterizedURLEditor(QWidget* parent) :
    QWidget(parent) {

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(QMargins());
    setLayout(layout);
    
    layout->addWidget(_line = new QLineEdit());
    connect(_line, SIGNAL(textChanged(const QString&)), SLOT(updateURL()));
}

void ParameterizedURLEditor::setURL(const ParameterizedURL& url) {
    _url = url;
    _line->setText(url.getURL().toString());
}

void ParameterizedURLEditor::updateURL() {
    _url = ParameterizedURL(_line->text());
    emit urlChanged(_url);
}
