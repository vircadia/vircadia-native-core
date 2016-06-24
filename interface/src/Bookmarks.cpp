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
#include <QJsonObject>
#include <QFile>
#include <QInputDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QStandardPaths>

#include <AddressManager.h>
#include <Application.h>
#include <OffscreenUi.h>

#include "MainWindow.h"
#include "Menu.h"
#include "InterfaceLogging.h"

#include "Bookmarks.h"
#include <QtQuick/QQuickWindow>


const QString Bookmarks::HOME_BOOKMARK = "Home";


Bookmarks::Bookmarks() {
    _bookmarksFilename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + BOOKMARKS_FILENAME;
    readFromFile();
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

void Bookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    // Add menus/actions
    auto bookmarkAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkLocation);
    QObject::connect(bookmarkAction, SIGNAL(triggered()), this, SLOT(bookmarkLocation()), Qt::QueuedConnection);
    auto setHomeAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::SetHomeLocation);
    QObject::connect(setHomeAction, SIGNAL(triggered()), this, SLOT(setHomeLocation()), Qt::QueuedConnection);
    _bookmarksMenu = menu->addMenu(MenuOption::Bookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteBookmark);
    QObject::connect(_deleteBookmarksAction, SIGNAL(triggered()), this, SLOT(deleteBookmark()), Qt::QueuedConnection);
    
    // Enable/Disable menus as needed
    enableMenuItems(_bookmarks.count() > 0);
    
    // Load bookmarks
    for (auto it = _bookmarks.begin(); it != _bookmarks.end(); ++it ) {
        QString bookmarkName = it.key();
        QString bookmarkAddress = it.value().toString();
        addLocationToMenu(menubar, bookmarkName, bookmarkAddress);
    }
}

void Bookmarks::bookmarkLocation() {
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
        removeLocationFromMenu(menubar, bookmarkName);
    }
    
    addLocationToMenu(menubar, bookmarkName, bookmarkAddress);
    insert(bookmarkName, bookmarkAddress);  // Overwrites any item with the same bookmarkName.
    
    enableMenuItems(true);
}

void Bookmarks::setHomeLocation() {
    Menu* menubar = Menu::getInstance();
    QString bookmarkName = HOME_BOOKMARK;
    auto addressManager = DependencyManager::get<AddressManager>();
    QString bookmarkAddress = addressManager->currentAddress().toString();

    addLocationToMenu(menubar, bookmarkName, bookmarkAddress);
    insert(bookmarkName, bookmarkAddress);  // Overwrites any item with the same bookmarkName.

    enableMenuItems(true);
}

void Bookmarks::teleportToBookmark() {
    QAction* action = qobject_cast<QAction*>(sender());
    QString address = action->data().toString();
    DependencyManager::get<AddressManager>()->handleLookupString(address);
}

void Bookmarks::deleteBookmark() {
    
    QStringList bookmarkList;
    QList<QAction*> menuItems = _bookmarksMenu->actions();
    for (int i = 0; i < menuItems.count(); i += 1) {
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
    
    removeLocationFromMenu(Menu::getInstance(), bookmarkName);
    remove(bookmarkName);
    
    if (_bookmarksMenu->actions().count() == 0) {
        enableMenuItems(false);
    }
}

void Bookmarks::enableMenuItems(bool enabled) {
    if (_bookmarksMenu) {
        _bookmarksMenu->setEnabled(enabled);
    }
    if (_deleteBookmarksAction) {
        _deleteBookmarksAction->setEnabled(enabled);
    }
}

void Bookmarks::addLocationToMenu(Menu* menubar, QString& name, QString& address) {
    QAction* teleportAction = _bookmarksMenu->newAction();
    teleportAction->setData(address);
    connect(teleportAction, SIGNAL(triggered()), this, SLOT(teleportToBookmark()));
    
    menubar->addActionToQMenuAndActionHash(_bookmarksMenu, teleportAction,
                                           name, 0, QAction::NoRole);
}

void Bookmarks::removeLocationFromMenu(Menu* menubar, QString& name) {
    menubar->removeAction(_bookmarksMenu, name);
}


