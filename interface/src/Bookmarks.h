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

#include <QMap>
#include <QObject>
#include <QPointer>

class QAction;
class QMenu;
class Menu;
class MenuWrapper;

class Bookmarks: public QObject {
    Q_OBJECT

public:
    Bookmarks();

    void setupMenus(Menu* menubar, MenuWrapper* menu);

    QString addressForBookmark(const QString& name) const;

    static const QString HOME_BOOKMARK;

private slots:
    void bookmarkLocation();
    void setHomeLocation();
    void teleportToBookmark();
    void deleteBookmark();
    
private:
    QVariantMap _bookmarks;  // { name: address, ... }
    
    QPointer<MenuWrapper> _bookmarksMenu;
    QPointer<QAction> _deleteBookmarksAction;

    const QString BOOKMARKS_FILENAME = "bookmarks.json";
    QString _bookmarksFilename;
    
    void insert(const QString& name, const QString& address);  // Overwrites any existing entry with same name.
    void remove(const QString& name);
    bool contains(const QString& name) const;

    void readFromFile();
    void persistToFile();

    void enableMenuItems(bool enabled);
    void addLocationToMenu(Menu* menubar, QString& name, QString& address);
    void removeLocationFromMenu(Menu* menubar, QString& name);
};

#endif // hifi_Bookmarks_h