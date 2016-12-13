//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TabletScriptingInterface.h"

#include <QtCore/QThread>

#include <OffscreenUi.h>
#include "QmlWrapper.h"

class TabletButtonProxy : public QmlWrapper {
    Q_OBJECT

public:
    TabletButtonProxy(QObject* qmlObject, QObject* parent = nullptr) : QmlWrapper(qmlObject, parent) {
        connect(qmlObject, SIGNAL(clicked()), this, SIGNAL(clicked()));
    }

signals:
    void clicked();
};

QObject* TabletScriptingInterface::getTablet(const QString& tabletId) {
    // AJT TODO: how the fuck do I get access to the toolbar qml?!? from here?
    return nullptr;
}

#include "TabletScriptingInterface.moc"
