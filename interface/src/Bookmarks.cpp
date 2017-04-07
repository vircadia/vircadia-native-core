//
//  Bookmarks.cpp
//  interface/src
//
//  Created by David Rowe on 13 Jan 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QAction>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include <Application.h>
#include <OffscreenUi.h>

#include "MainWindow.h"
#include "Menu.h"
#include "InterfaceLogging.h"

#include "Bookmarks.h"

Bookmarks::Bookmarks() :
_isMenuSorted(false)
{
}

void Bookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    // Enable/Disable menus as needed
    enableMenuItems(_bookmarks.count() > 0);

    // Load Bookmarks
    for (auto it = _bookmarks.begin(); it != _bookmarks.end(); ++it) {
        QString bookmarkName = it.key();
        QString bookmarkAddress = it.value().toString();
        addBookmarkToMenu(menubar, bookmarkName, bookmarkAddress);
    }
}

void Bookmarks::deleteBookmark() {
    QStringList bookmarkList;
    QList<QAction*> menuItems = _bookmarksMenu->actions();
    for (int i = 0; i < menuItems.count(); ++i) {
        bookmarkList.append(menuItems[i]->text());
    }

    bool ok = false;
    auto bookmarkName = OffscreenUi::getItem(OffscreenUi::ICON_PLACEMARK, "Delete Bookmark", "Select the bookmark to delete", bookmarkList, 0, false, &ok);
    if (!ok) {
        return;
    }

    bookmarkName = bookmarkName.trimmed();
    if (bookmarkName.length() == 0) {
        return;
    }

    removeBookmarkFromMenu(Menu::getInstance(), bookmarkName);
    remove(bookmarkName);

    if (_bookmarksMenu->actions().count() == 0) {
        enableMenuItems(false);
    }
}

void Bookmarks::addBookmarkToFile(const QString& bookmarkName, const QString& bookmarkAddress) {
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

void Bookmarks::insert(const QString& name, const QString& address) {
    _bookmarks.insert(name, address);

    if (contains(name)) {
        qCDebug(interfaceapp) << "Added bookmark:" << name << "," << address;
        persistToFile();
    } else {
        qWarning() << "Couldn't add bookmark: " << name << "," << address;
    }
}

void Bookmarks::remove(const QString& name) {
    _bookmarks.remove(name);

    if (!contains(name)) {
        qCDebug(interfaceapp) << "Deleted bookmark:" << name;
        persistToFile();
    } else {
        qWarning() << "Couldn't delete bookmark:" << name;
    }
}

bool Bookmarks::contains(const QString& name) const {
    return _bookmarks.contains(name);
}

bool Bookmarks::sortOrder(QAction* a, QAction* b) {
    return a->text().toLower().localeAwareCompare(b->text().toLower()) < 0;
}

void Bookmarks::sortActions(Menu* menubar, MenuWrapper* menu) {
    QList<QAction*> actions = menu->actions();
    qSort(actions.begin(), actions.end(), sortOrder);
    for (QAction* action : menu->actions()) {
        menu->removeAction(action);
    }
    for (QAction* action : actions) {
        menu->addAction(action);
    }
    _isMenuSorted = true;
}

int Bookmarks::getMenuItemLocation(QList<QAction*> actions, const QString& name) const {
    int menuItemLocation = 0;
    for (QAction* action : actions) {
        if (name.toLower().localeAwareCompare(action->text().toLower()) < 0) {
            menuItemLocation = actions.indexOf(action);
            break;
        }
    }
    return menuItemLocation;
}

QString Bookmarks::addressForBookmark(const QString& name) const {
    return _bookmarks.value(name).toString();
}

void Bookmarks::readFromFile() {
    QFile loadFile(_bookmarksFilename);

    if (!loadFile.exists()) {
        // User has not yet saved bookmarks
        return;
    }

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open bookmarks file for reading");
        return;
    }

    QByteArray data = loadFile.readAll();
    QJsonDocument json(QJsonDocument::fromJson(data));
    _bookmarks = json.object().toVariantMap();
}

void Bookmarks::persistToFile() {
    QFile saveFile(_bookmarksFilename);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open bookmarks file for writing");
        return;
    }

    QJsonDocument json(QJsonObject::fromVariantMap(_bookmarks));
    QByteArray data = json.toJson();
    saveFile.write(data);
}

void Bookmarks::enableMenuItems(bool enabled) {
    if (_bookmarksMenu) {
        _bookmarksMenu->setEnabled(enabled);
    }
    if (_deleteBookmarksAction) {
        _deleteBookmarksAction->setEnabled(enabled);
    }
}

void Bookmarks::removeBookmarkFromMenu(Menu* menubar, const QString& name) {
    menubar->removeAction(_bookmarksMenu, name);
}
