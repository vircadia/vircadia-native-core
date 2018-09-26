//
//  QUrlAncestry.cpp
//  libraries/shared/src/
//
//  Created by Kerry Ivan Kurian on 10/12/18.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QUrlAncestry.h"


QUrlAncestry::QUrlAncestry(const QUrl& resource, const QUrl& referrer) {
    this->append(referrer);
    this->append(resource);
}

QUrlAncestry::QUrlAncestry(
    const QUrl& resource,
    const QUrlAncestry& ancestors) : QVector<QUrl>(ancestors)
{
    this->append(resource);
}

void QUrlAncestry::toJson(QJsonArray& array) const {
    for (auto const& qurl : *this) {
        array.append(qurl.toDisplayString());
    }
}

const QUrl QUrlAncestry::url() const {
    return this->last();
}
