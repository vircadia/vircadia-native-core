//
//  QUrlAncestry.h
//  libraries/shared/src/
//
//  Created by Kerry Ivan Kurian on 10/12/18.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_QUrlAncestry_H
#define hifi_QUrlAncestry_H

#include <QJsonArray>
#include <QUrl>
#include <QVector>


class QUrlAncestry : public QVector<QUrl> {
public:
    QUrlAncestry() {}
    QUrlAncestry(const QUrl& resource, const QUrl& referrer = QUrl("__NONE__"));
    QUrlAncestry(const QUrl& resource, const QUrlAncestry& ancestors);

    void toJson(QJsonArray& array) const;
    const QUrl url() const;
};


#endif  // hifi_QUrlVector_H
