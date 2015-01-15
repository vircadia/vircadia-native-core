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

class Bookmarks: public QObject {
    Q_OBJECT

public:
    Bookmarks();

    void insert(const QString& name, const QString& address);  // Overwrites any existing entry with same name.
    void remove(const QString& name);
    bool contains(const QString& name) const;

    QVariantMap* getBookmarks() { return &_bookmarks; };

private:
    QVariantMap _bookmarks;  // { name: address, ... }

    const QString BOOKMARKS_FILENAME = "bookmarks.json";
    QString _bookmarksFilename;
    void readFromFile();
    void persistToFile();
};

#endif // hifi_Bookmarks_h