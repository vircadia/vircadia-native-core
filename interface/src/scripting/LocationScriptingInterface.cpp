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
    MyAvatar* applicationAvatar = Application::getInstance()->getAvatar();
    return AddressManager::pathForPositionAndOrientation(applicationAvatar->getPosition(),
                                                         true, applicationAvatar->getOrientation());
}

QString LocationScriptingInterface::getHostname() {
    return NodeList::getInstance()->getDomainHandler().getHostname();
}

QString LocationScriptingInterface::getDomainID() const {
    return uuidStringWithoutCurlyBraces(NodeList::getInstance()->getDomainHandler().getUUID());
}

void LocationScriptingInterface::assign(const QString& url) {
    QMetaObject::invokeMethod(&AddressManager::getInstance(), "handleLookupString", Q_ARG(const QString&, url));
}

QScriptValue LocationScriptingInterface::locationGetter(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(getInstance());
}

QScriptValue LocationScriptingInterface::locationSetter(QScriptContext* context, QScriptEngine* engine) {
    LocationScriptingInterface::getInstance()->assign(context->argument(0).toString());
    return QScriptValue::UndefinedValue;
}
