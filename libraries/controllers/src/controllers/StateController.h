//
//  StateController.h
//  controllers/src/controllers
//
//  Created by Sam Gateau on 2015-10-27.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StateController_h
#define hifi_StateController_h

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "InputDevice.h"

namespace controller {

class StateController : public QObject, public InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)

public:
    const QString& getName() const { return _name; }

    // Device functions
    virtual void buildDeviceProxy(DeviceProxy::Pointer proxy) override;
    virtual QString getDefaultMappingConfig() override { return QString(); }
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;

    StateController();
    virtual ~StateController();

    using ReadLambda = std::function<float()>;
    using NamedReadLambda = QPair<QString, ReadLambda>;

    void addInputVariant(QString name, ReadLambda& lambda);

    QVector<NamedReadLambda> namedReadLambdas;
    QVector<Input::NamedPair> availableInputs;
};

}

#endif // hifi_StateController_h