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
    _bookmarksFilename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + LOCATIONBOOKMARKS_FILENAME;
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

    // Legacy Location to Bookmark.

    // Enable/Disable menus as needed
    enableMenuItems(_bookmarks.count() > 0);

    // Load Bookmarks
    for (auto it = _bookmarks.begin(); it != _bookmarks.end(); ++it) {
        QString bookmarkName = it.key();
        QString bookmarkAddress = it.value().toString();
        addBookmarkToMenu(menubar, bookmarkName, bookmarkAddress);
    }

    Bookmarks::sortActions(menubar, _bookmarksMenu);
}

void LocationBookmarks::setHomeLocation() {
    auto addressManager = DependencyManager::get<AddressManager>();
    QString bookmarkAddress = addressManager->currentAddress().toString();

    Bookmarks::addBookmarkToFile(HOME_BOOKMARK, bookmarkAddress);
}

void LocationBookmarks::setHomeLocationToAddress(const QVariant& address) {
    Bookmarks::insert("Home", address);
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
    Bookmarks::addBookmarkToFile(bookmarkName, bookmarkAddress);
}

void LocationBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& address) {
    QAction* teleportAction = _bookmarksMenu->newAction();
    teleportAction->setData(address);
    connect(teleportAction, SIGNAL(triggered()), this, SLOT(teleportToBookmark()));
    if (!_isMenuSorted) {
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, teleportAction, name, 0, QAction::NoRole);
    } else {
        // TODO: this is aggressive but other alternatives have proved less fruitful so far.
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, teleportAction, name, 0, QAction::NoRole);
        Bookmarks::sortActions(menubar, _bookmarksMenu);
    }
}