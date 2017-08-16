//
//  Baker.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/14/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelBakingLoggingCategory.h"

#include "Baker.h"

void Baker::handleError(const QString& error) {
    qCCritical(model_baking).noquote() << error;
    _errorList.append(error);
    emit finished();
}

void Baker::handleErrors(const QStringList& errors) {
    // we're appending errors, presumably from a baking operation we called
    // add those to our list and emit that we are finished
    _errorList.append(errors);
    emit finished();
}

void Baker::handleWarning(const QString& warning) {
    qCWarning(model_baking).noquote() << warning;
    _warningList.append(warning);
}
