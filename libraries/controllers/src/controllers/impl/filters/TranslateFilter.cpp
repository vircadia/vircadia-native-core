//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TranslateFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <StreamUtils.h>

using namespace controller;

bool TranslateFilter::parseParameters(const QJsonValue& parameters) {
    static const QString JSON_TRANSLATE = QStringLiteral("translate");
    bool result = parseVec3Parameter(parameters, JSON_TRANSLATE, _translate);
    qDebug() << __FUNCTION__ << "_translate:" << _translate;
    return result;
}
