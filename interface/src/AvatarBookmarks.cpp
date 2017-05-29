//
//  AvatarBookmarks.cpp
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
#include <QQmlContext>

#include <Application.h>
#include <OffscreenUi.h>
#include <avatar/AvatarManager.h>

#include "MainWindow.h"
#include "Menu.h"

#include "AvatarBookmarks.h"
#include <QtQuick/QQuickWindow>

AvatarBookmarks::AvatarBookmarks() {
    _bookmarksFilename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + AVATARBOOKMARKS_FILENAME;
    readFromFile();
}

void AvatarBookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    // Add menus/actions
    auto bookmarkAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkAvatar);
    QObject::connect(bookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()), Qt::QueuedConnection);
    _bookmarksMenu = menu->addMenu(MenuOption::AvatarBookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteAvatarBookmark);
    QObject::connect(_deleteBookmarksAction, SIGNAL(triggered()), this, SLOT(deleteBookmark()), Qt::QueuedConnection);

    Bookmarks::setupMenus(menubar, menu);
    Bookmarks::sortActions(menubar, _bookmarksMenu);
}

void AvatarBookmarks::changeToBookmarkedAvatar() {
    QAction* action = qobject_cast<QAction*>(sender());
    const QString& address = action->data().toString();

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->useFullAvatarURL(address);
}

void AvatarBookmarks::addBookmark() {
    bool ok = false;
    auto bookmarkName = OffscreenUi::getText(OffscreenUi::ICON_PLACEMARK, "Bookmark Avatar", "Name", QString(), &ok);
    if (!ok) {
        return;
    }

    bookmarkName = bookmarkName.trimmed().replace(QRegExp("(\r\n|[\r\n\t\v ])+"), " ");
    if (bookmarkName.length() == 0) {
        return;
    }

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    const QString& bookmarkAddress = myAvatar->getSkeletonModelURL().toString();
    Bookmarks::addBookmarkToFile(bookmarkName, bookmarkAddress);
}

void AvatarBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QString& address) {
    QAction* changeAction = _bookmarksMenu->newAction();
    changeAction->setData(address);
    connect(changeAction, SIGNAL(triggered()), this, SLOT(changeToBookmarkedAvatar()));
    if (!_isMenuSorted) {
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
    } else {
        // TODO: this is aggressive but other alternatives have proved less fruitful so far.
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
        Bookmarks::sortActions(menubar, _bookmarksMenu);
    }
}
