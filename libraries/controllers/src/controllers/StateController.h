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
    using Pointer = std::shared_ptr<StateController>;
    using ReadLambda = std::function<float()>;
    using NamedReadLambda = QPair<QString, ReadLambda>;

    static void setStateVariables(const QStringList& stateVariables);

    StateController();

    const QString& getName() const { return _name; }

    // Device functions
    Input::NamedVector getAvailableInputs() const override;

    void setInputVariant(const QString& name, ReadLambda lambda);

    EndpointPointer createEndpoint(const Input& input) const override;

protected:
    QHash<QString, ReadLambda> _namedReadLambdas;
};

}

#endif // hifi_StateController_h