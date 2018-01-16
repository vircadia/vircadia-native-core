//
//  AvatarEntitiesBookmarks.cpp
//  interface/src
//
//  Created by Dante Ruiz on /01/18.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QQmlContext>
#include <QList>

#include <Application.h>
#include <OffscreenUi.h>
#include <avatar/AvatarManager.h>

#include "MainWindow.h"
#include "Menu.h"
#include "AvatarEntitiesBookmarks.h"
#include "InterfaceLogging.h"

#include "QVariantGLM.h"

#include <QtQuick/QQuickWindow>

AvatarEntitiesBookmarks::AvatarEntitiesBookmarks() {
    _bookmarksFilename = PathUtils::getAppDataPath() + "/" + AVATAR_ENTITIES_BOOKMARKS_FILENAME;
    readFromFile();
}

void AvatarEntitiesBookmarks::readFromFile() {
    QString oldConfigPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + AVATAR_ENTITIES_BOOKMARKS_FILENAME;
    QFile oldConfig(oldConfigPath);
    // I imagine that in a year from now, this code for migrating (as well as the two lines above)
    // may be removed since all bookmarks should have been migrated by then
    // - Robbie Uvanni (6.8.2017)
    if (oldConfig.exists()) {
        if (QDir().rename(oldConfigPath, _bookmarksFilename)) {
            qCDebug(interfaceapp) << "Successfully migrated" << AVATAR_ENTITIES_BOOKMARKS_FILENAME;
        } else {
            qCDebug(interfaceapp) << "Failed to migrate" << AVATAR_ENTITIES_BOOKMARKS_FILENAME;
        }
    }

    Bookmarks::readFromFile();
}

void AvatarEntitiesBookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    auto bookmarkAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkAvatarEntities);
    QObject::connect(bookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()), Qt::QueuedConnection);
    _bookmarksMenu = menu->addMenu(MenuOption::AvatarEntitiesBookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteAvatarBookmark);
    QObject::connect(_deleteBookmarksAction, SIGNAL(triggered()), this, SLOT(deleteBookmark()), Qt::QueuedConnection);

    for (auto it = _bookmarks.begin(); it != _bookmarks.end(); ++it) {
        addBookmarkToMenu(menubar, it.key(), it.value());
    }

    Bookmarks::sortActions(menubar, _bookmarksMenu);
}

void AvatarEntitiesBookmarks::applyBookmarkedAvatarEntities() {
}

void AvatarEntitiesBookmarks::addBookmark() {
}

void AvatarEntitiesBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) {
    QAction* changeAction = _bookmarksMenu->newAction();
    changeAction->setData(bookmark);
    connect(changeAction, SIGNAL(triggered()), this, SLOT(applyBookedAvatarEntities()));
    if (!_isMenuSorted) {
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
    } else {
        // TODO: this is aggressive but other alternatives have proved less fruitful so far.
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
        Bookmarks::sortActions(menubar, _bookmarksMenu);
    }
}
