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
#include "InterfaceLogging.h"

#include <QtQuick/QQuickWindow>

AvatarBookmarks::AvatarBookmarks() {
    _bookmarksFilename = PathUtils::getAppDataPath() + "/" + AVATARBOOKMARKS_FILENAME;
    readFromFile();
}

void AvatarBookmarks::readFromFile() {
    // migrate old avatarbookmarks.json, used to be in 'local' folder on windows
    QString oldConfigPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + AVATARBOOKMARKS_FILENAME;
    QFile oldConfig(oldConfigPath);
    
    // I imagine that in a year from now, this code for migrating (as well as the two lines above)
    // may be removed since all bookmarks should have been migrated by then
    // - Robbie Uvanni (6.8.2017)
    if (oldConfig.exists()) {
        if (QDir().rename(oldConfigPath, _bookmarksFilename)) {
            qCDebug(interfaceapp) << "Successfully migrated" << AVATARBOOKMARKS_FILENAME;
        } else {
            qCDebug(interfaceapp) << "Failed to migrate" << AVATARBOOKMARKS_FILENAME;
        }
    } 
    
    Bookmarks::readFromFile();     
}

void AvatarBookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    // Add menus/actions
    auto bookmarkAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkAvatar);
    QObject::connect(bookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()), Qt::QueuedConnection);
    _bookmarksMenu = menu->addMenu(MenuOption::AvatarBookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteAvatarBookmark);
    QObject::connect(_deleteBookmarksAction, SIGNAL(triggered()), this, SLOT(deleteBookmark()), Qt::QueuedConnection);

    for (auto it = _bookmarks.begin(); it != _bookmarks.end(); ++it) {
        addBookmarkToMenu(menubar, it.key(), it.value());
    }

    Bookmarks::sortActions(menubar, _bookmarksMenu);
}

void AvatarBookmarks::changeToBookmarkedAvatar() {
    QAction* action = qobject_cast<QAction*>(sender());
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();


    // TODO: Phase this out eventually.
    if (action->data().type() == QVariant::String) {
        // Legacy avatar bookmark.

        myAvatar->useFullAvatarURL(action->data().toString());
        qCDebug(interfaceapp) << " Using Legacy Avatar Bookmark ";
    }
    else {
        // TODO: this is where the entry is interpreted.
        const QMap<QString, QVariant> bookmark = action->data().toMap();
        // Not magic value. This is the current made version, and if it changes this interpreter should be updated to handle the new one separately.
        if (bookmark.value(ENTRY_VERSION) == 3) {

            myAvatar->useFullAvatarURL(bookmark.value(ENTRY_AVATAR_URL).toString());
            myAvatar->setAttachmentsVariant(bookmark.value(ENTRY_AVATAR_ATTACHMENTS).toList());
        }
        else {
            qCDebug(interfaceapp) << " Bookmark entry does not match client version, make sure client has a handler for the new AvatarBookmark";
        }
    }

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

    const QString& avatarUrl = myAvatar->getSkeletonModelURL().toString();
    const QVariantList& attachments = myAvatar->getAttachmentsVariant();

    // If Avatar attachments ever change, this is where to update them, when saving remember to also append to AVATAR_BOOKMARK_VERSION
    QVariantMap *bookmark = new QVariantMap;
    bookmark->insert(ENTRY_VERSION, AVATAR_BOOKMARK_VERSION);
    bookmark->insert(ENTRY_AVATAR_URL, avatarUrl);
    bookmark->insert(ENTRY_AVATAR_ATTACHMENTS, attachments);
    
    Bookmarks::addBookmarkToFile(bookmarkName, *bookmark);
}

void AvatarBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) {
    QAction* changeAction = _bookmarksMenu->newAction();
    changeAction->setData(bookmark);
    connect(changeAction, SIGNAL(triggered()), this, SLOT(changeToBookmarkedAvatar()));
    if (!_isMenuSorted) {
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
    } else {
        // TODO: this is aggressive but other alternatives have proved less fruitful so far.
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
        Bookmarks::sortActions(menubar, _bookmarksMenu);
    }
}
