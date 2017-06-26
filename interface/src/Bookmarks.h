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

    virtual void setupMenus(Menu* menubar, MenuWrapper* menu) = 0;
    QString addressForBookmark(const QString& name) const;

protected:
    void addBookmarkToFile(const QString& bookmarkName, const QVariant& bookmark);
    virtual void addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) = 0;
    void enableMenuItems(bool enabled);
    virtual void readFromFile();
    void insert(const QString& name, const QVariant& address);  // Overwrites any existing entry with same name.
    void sortActions(Menu* menubar, MenuWrapper* menu);
    int getMenuItemLocation(QList<QAction*> actions, const QString& name) const;
    
    bool contains(const QString& name) const;
    
    QVariantMap _bookmarks;  // { name: url, ... }
    QPointer<MenuWrapper> _bookmarksMenu;
    QPointer<QAction> _deleteBookmarksAction;
    QString _bookmarksFilename;
    bool _isMenuSorted;

protected slots:
    void deleteBookmark();

private:
    void remove(const QString& name);
    static bool sortOrder(QAction* a, QAction* b);

    void persistToFile();

    void removeBookmarkFromMenu(Menu* menubar, const QString& name);
};

#endif // hifi_Bookmarks_h
