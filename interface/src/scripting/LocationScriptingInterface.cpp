//
//  LocationScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>

#include "NodeList.h"

#include "LocationScriptingInterface.h"

LocationScriptingInterface* LocationScriptingInterface::getInstance() {
    static LocationScriptingInterface sharedInstance;
    return &sharedInstance;
}

bool LocationScriptingInterface::isConnected() {
    return NodeList::getInstance()->getDomainHandler().isConnected();
}

QString LocationScriptingInterface::getHref() {
    return getProtocol() + "//" + getHostname() + getPathname();
}

QString LocationScriptingInterface::getPathname() {
    const glm::vec3& position = Application::getInstance()->getAvatar()->getPosition();
    QString path;
    path.sprintf("/%.4f,%.4f,%.4f", position.x, position.y, position.z);
    return path;
}

QString LocationScriptingInterface::getHostname() {
    return NodeList::getInstance()->getDomainHandler().getHostname();
}

void LocationScriptingInterface::assign(const QString& url) {
    QMetaObject::invokeMethod(Menu::getInstance(), "goToURL", Q_ARG(const QString&, url));
}

QScriptValue LocationScriptingInterface::locationGetter(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(getInstance());
}

QScriptValue LocationScriptingInterface::locationSetter(QScriptContext* context, QScriptEngine* engine) {
    LocationScriptingInterface::getInstance()->assign(context->argument(0).toString());
    return QScriptValue::UndefinedValue;
}
