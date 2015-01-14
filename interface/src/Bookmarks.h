//
//  Bookmarks.h
//  interface/src
//
//  Created by David Rowe on 13 Jan 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Bookmarks_h
#define hifi_Bookmarks_h

#include <QJsonObject>
#include <QDebug>
#include <QMap>
#include <QObject>
#include <QStringList>

class Bookmarks: public QObject {
    Q_OBJECT

public:
    Bookmarks();

    void insert(QString name, QString address);  // Overwrites any existing entry with same name.
    void remove(QString name);

    bool contains(QString name);
    bool isValidName(QString name);

private:
    QMap<QString, QJsonObject> _bookmarks;  // key: { name: string, address: string }
                                            // key is a lowercase copy of name, used to make the bookmarks case insensitive.
    const QRegExp _nameRegExp = QRegExp("^[\\w\\-]+$");
};

#endif // hifi_Bookmarks_h