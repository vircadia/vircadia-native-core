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

#include "MainWindow.h"
#include "Menu.h"

#include "Bookmarks.h"

Bookmarks::Bookmarks() {
    _bookmarksFilename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + BOOKMARKS_FILENAME;
    readFromFile();
}

void Bookmarks::insert(const QString& name, const QString& address) {
    _bookmarks.insert(name, address);

    if (contains(name)) {
        qDebug() << "Added bookmark:" << name << "," << address;
        persistToFile();
    } else {
        qWarning() << "Couldn't add bookmark: " << name << "," << address;
    }
}

void Bookmarks::remove(const QString& name) {
    _bookmarks.remove(name);

    if (!contains(name)) {
        qDebug() << "Deleted bookmark:" << name;
        persistToFile();
    } else {
        qWarning() << "Couldn't delete bookmark:" << name;
    }
}

bool Bookmarks::contains(const QString& name) const {
    return _bookmarks.contains(name);
}

void Bookmarks::readFromFile() {
    QFile loadFile(_bookmarksFilename);

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

void Bookmarks::setupMenus(Menu* menubar, QMenu* menu) {
    // Add menus/actions
    menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkLocation, 0,
                                           this, SLOT(bookmarkLocation()));
    _bookmarksMenu = menu->addMenu(MenuOption::Bookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteBookmark, 0,
                                                                    this, SLOT(deleteBookmark()));
    
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
    QInputDialog bookmarkLocationDialog(qApp->getWindow());
    bookmarkLocationDialog.setWindowTitle("Bookmark Location");
    bookmarkLocationDialog.setLabelText("Name:");
    bookmarkLocationDialog.setInputMode(QInputDialog::TextInput);
    bookmarkLocationDialog.resize(400, 200);
    
    if (bookmarkLocationDialog.exec() == QDialog::Rejected) {
        return;
    }
    
    QString bookmarkName = bookmarkLocationDialog.textValue().trimmed();
    bookmarkName = bookmarkName.replace(QRegExp("(\r\n|[\r\n\t\v ])+"), " ");
    if (bookmarkName.length() == 0) {
        return;
    }
    
    auto addressManager = DependencyManager::get<AddressManager>();
    QString bookmarkAddress = addressManager->currentAddress().toString();
    
    Menu* menubar = Menu::getInstance();
    if (contains(bookmarkName)) {
        QMessageBox duplicateBookmarkMessage;
        duplicateBookmarkMessage.setIcon(QMessageBox::Warning);
        duplicateBookmarkMessage.setText("The bookmark name you entered already exists in your list.");
        duplicateBookmarkMessage.setInformativeText("Would you like to overwrite it?");
        duplicateBookmarkMessage.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        duplicateBookmarkMessage.setDefaultButton(QMessageBox::Yes);
        if (duplicateBookmarkMessage.exec() == QMessageBox::No) {
            return;
        }
        removeLocationFromMenu(menubar, bookmarkName);
    }
    
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
    
    QInputDialog deleteBookmarkDialog(qApp->getWindow());
    deleteBookmarkDialog.setWindowTitle("Delete Bookmark");
    deleteBookmarkDialog.setLabelText("Select the bookmark to delete");
    deleteBookmarkDialog.resize(400, 400);
    deleteBookmarkDialog.setOption(QInputDialog::UseListViewForComboBoxItems);
    deleteBookmarkDialog.setComboBoxItems(bookmarkList);
    deleteBookmarkDialog.setOkButtonText("Delete");
    
    if (deleteBookmarkDialog.exec() == QDialog::Rejected) {
        return;
    }
    
    QString bookmarkName = deleteBookmarkDialog.textValue().trimmed();
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
    QAction* teleportAction = new QAction(_bookmarksMenu);
    teleportAction->setData(address);
    connect(teleportAction, SIGNAL(triggered()), this, SLOT(teleportToBookmark()));
    
    menubar->addActionToQMenuAndActionHash(_bookmarksMenu, teleportAction,
                                           name, 0, QAction::NoRole);
}

void Bookmarks::removeLocationFromMenu(Menu* menubar, QString& name) {
    menubar->removeAction(_bookmarksMenu, name);
}


