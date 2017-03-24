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

#include <Application.h>
#include <OffscreenUi.h>

#include "MainWindow.h"
#include "Menu.h"

#include <avatar/AvatarManager.h>

#include "AvatarBookmarks.h"
#include <QtQuick/QQuickWindow>

AvatarBookmarks::AvatarBookmarks() {
    _bookmarksFilename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + AvatarBookmarks_FILENAME;
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

    // connect bookmarkAvatarButton in AvatarPreferencesDialog.qml

    // TODO: attempt at connecting to bookmarkAvatarSignal in AvatarPreferencesDialog.qml
    // the OffscreenUi doesn't seem available this early to recurse through to find the root object where the signal is declared
    // I've added a delay for now

    // The OffscreenUi also doesn't create the object till it is shown first, so I'm forcing it to show so the object exists
    QTimer::singleShot(2000, [&] {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->show(QString("hifi/dialogs/AvatarPreferencesDialog.qml"), "AvatarPreferencesDialog");
        auto bookmarkAvatarButton = offscreenUi->getRootItem()->findChild<QQuickItem*>("avatarPreferencesRoot");
        if (bookmarkAvatarButton) {
            QObject::connect(bookmarkAvatarButton, SIGNAL(bookmarkAvatarSignal()), this, SLOT(addBookmark()));
        }
    });
}

void AvatarBookmarks::changeToBookmarkedAvatar() {
    QAction* action = qobject_cast<QAction*>(sender());
    const QString& address = action->data().toString();

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->useFullAvatarURL(address);
}

void AvatarBookmarks::addBookmark() {
    // TODO:  if you press the Bookmark Avatar button in the dialog it seems to maintain focus.
    // Clicking afterwards results in multiple calls
    //  hide enforced till cause is determined 
    DependencyManager::get<OffscreenUi>()->hide(QString("AvatarPreferencesDialog"));

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

    Menu* menubar = Menu::getInstance();
    if (contains(bookmarkName)) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        auto duplicateBookmarkMessage = offscreenUi->createMessageBox(OffscreenUi::ICON_WARNING, "Duplicate Bookmark",
            "The bookmark name you entered already exists in your list.",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        duplicateBookmarkMessage->setProperty("informativeText", "Would you like to overwrite it?");

        auto result = offscreenUi->waitForMessageBoxResult(duplicateBookmarkMessage);
        if (result != QMessageBox::Yes) {
            return;
        }
        removeBookmarkFromMenu(menubar, bookmarkName);
    }

    addBookmarkToMenu(menubar, bookmarkName, bookmarkAddress);
    insert(bookmarkName, bookmarkAddress);  // Overwrites any item with the same bookmarkName.

    enableMenuItems(true);
}

void AvatarBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QString& address) {
    QAction* changeAction = _bookmarksMenu->newAction();
    changeAction->setData(address);
    connect(changeAction, SIGNAL(triggered()), this, SLOT(changeToBookmarkedAvatar()));

    menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction,
        name, 0, QAction::NoRole);
}
