//
//  LocationBookmarks.cpp
//  interface/src
//
//  Created by Triplelexx on 23/03/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include <AddressManager.h>
#include <Application.h>
#include <OffscreenUi.h>

#include "MainWindow.h"
#include "Menu.h"

#include "LocationBookmarks.h"
#include <QtQuick/QQuickWindow>

const QString LocationBookmarks::HOME_BOOKMARK = "Home";

LocationBookmarks::LocationBookmarks() {
    _bookmarksFilename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + LocationBookmarks_FILENAME;
    readFromFile();
}

void LocationBookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    // Add menus/actions
    auto bookmarkAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkLocation);
    QObject::connect(bookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()), Qt::QueuedConnection);
    auto setHomeAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::SetHomeLocation);
    QObject::connect(setHomeAction, SIGNAL(triggered()), this, SLOT(setHomeLocation()), Qt::QueuedConnection);
    _bookmarksMenu = menu->addMenu(MenuOption::LocationBookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteBookmark);
    QObject::connect(_deleteBookmarksAction, SIGNAL(triggered()), this, SLOT(deleteBookmark()), Qt::QueuedConnection);
    
    Bookmarks::setupMenus(menubar, menu);
}

void LocationBookmarks::setHomeLocation() {
    Menu* menubar = Menu::getInstance();
    QString bookmarkName = HOME_BOOKMARK;
    auto addressManager = DependencyManager::get<AddressManager>();
    QString bookmarkAddress = addressManager->currentAddress().toString();

    addBookmarkToMenu(menubar, bookmarkName, bookmarkAddress);
    insert(bookmarkName, bookmarkAddress);  // Overwrites any item with the same bookmarkName.

    enableMenuItems(true);
}

void LocationBookmarks::teleportToBookmark() {
    QAction* action = qobject_cast<QAction*>(sender());
    QString address = action->data().toString();
    DependencyManager::get<AddressManager>()->handleLookupString(address);
}

void LocationBookmarks::addBookmark() {
    bool ok = false;
    auto bookmarkName = OffscreenUi::getText(OffscreenUi::ICON_PLACEMARK, "Bookmark Location", "Name", QString(), &ok);
    if (!ok) {
        return;
    }

    bookmarkName = bookmarkName.trimmed().replace(QRegExp("(\r\n|[\r\n\t\v ])+"), " ");
    if (bookmarkName.length() == 0) {
        return;
    }

    auto addressManager = DependencyManager::get<AddressManager>();
    QString bookmarkAddress = addressManager->currentAddress().toString();
    Bookmarks::addBookmark(bookmarkName, bookmarkAddress);
}

void LocationBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QString& address) {
    QAction* teleportAction = _bookmarksMenu->newAction();
    teleportAction->setData(address);
    connect(teleportAction, SIGNAL(triggered()), this, SLOT(teleportToBookmark()));
    
    menubar->addActionToQMenuAndActionHash(_bookmarksMenu, teleportAction,
                                           name, 0, QAction::NoRole);
}
