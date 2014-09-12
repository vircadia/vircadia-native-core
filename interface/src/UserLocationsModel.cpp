//
//  UserLocationsModel.cpp
//  interface/src
//
//  Created by Ryan Huffman on 06/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>

#include "AccountManager.h"
#include "Application.h"
#include "UserLocationsModel.h"

static const QString LOCATIONS_GET = "/api/v1/locations";
static const QString LOCATION_UPDATE_OR_DELETE = "/api/v1/locations/%1";

UserLocation::UserLocation(const QString& id, const QString& name, const QString& address) :
    _id(id),
    _name(name),
    _address(address),
    _previousName(name),
    _updating(false) {
}

void UserLocation::requestRename(const QString& newName) {
    if (!_updating && newName.toLower() != _name) {
        _updating = true;

        JSONCallbackParameters callbackParams(this, "handleRenameResponse", this, "handleRenameError");
        
        QJsonObject jsonNameObject;
        jsonNameObject.insert("name", newName);
        
        QJsonObject locationObject;
        locationObject.insert("location", jsonNameObject);
        
        QJsonDocument jsonDocument(jsonNameObject);
        AccountManager::getInstance().authenticatedRequest(LOCATION_UPDATE_OR_DELETE.arg(_id),
                                                           QNetworkAccessManager::PutOperation,
                                                           callbackParams,
                                                           jsonDocument.toJson());
        _previousName = _name;
        _name = newName;

        emit updated(_name);
    }
}

void UserLocation::handleRenameResponse(const QJsonObject& responseData) {
    _updating = false;

    QJsonValue status = responseData["status"];
    if (!status.isUndefined() && status.toString() == "success") {
        qDebug() << responseData;
        QString updatedName = responseData["data"].toObject()["location"].toObject()["name"].toString();
        qDebug() << "The updated name is" << updatedName;
        _name = updatedName;
    } else {
        _name = _previousName;

        QString msg = "There was an error renaming location '" + _name + "'";

        QJsonValue data = responseData["data"];
        if (!data.isUndefined()) {
            QJsonValue nameError = data.toObject()["name"];
            if (!nameError.isUndefined()) {
                msg += ": " + nameError.toString();
            }
        }
        qDebug() << msg;
        QMessageBox::warning(Application::getInstance()->getWindow(), "Error", msg);
    }

    emit updated(_name);
}

void UserLocation::handleRenameError(QNetworkReply& errorReply) {
    _updating = false;

    QString msg = "There was an error renaming location '" + _name + "': " + errorReply.errorString();
    qDebug() << msg;
    QMessageBox::warning(Application::getInstance()->getWindow(), "Error", msg);

    emit updated(_name);
}

void UserLocation::requestDelete() {
    if (!_updating) {
        _updating = true;

        JSONCallbackParameters callbackParams(this, "handleDeleteResponse", this, "handleDeleteError");
        AccountManager::getInstance().authenticatedRequest(LOCATION_UPDATE_OR_DELETE.arg(_id),
                                                           QNetworkAccessManager::DeleteOperation,
                                                           callbackParams);
    }
}

void UserLocation::handleDeleteResponse(const QJsonObject& responseData) {
    _updating = false;

    QJsonValue status = responseData["status"];
    if (!status.isUndefined() && status.toString() == "success") {
        emit deleted(_name);
    } else {
        QString msg = "There was an error deleting location '" + _name + "'";
        qDebug() << msg;
        QMessageBox::warning(Application::getInstance()->getWindow(), "Error", msg);
    }
}

void UserLocation::handleDeleteError(QNetworkReply& errorReply) {
    _updating = false;

    QString msg = "There was an error deleting location '" + _name + "': " + errorReply.errorString();
    qDebug() << msg;
    QMessageBox::warning(Application::getInstance()->getWindow(), "Error", msg);
}

UserLocationsModel::UserLocationsModel(QObject* parent) :
    QAbstractListModel(parent),
    _updating(false) {

    refresh();
}

UserLocationsModel::~UserLocationsModel() {
    qDeleteAll(_locations);
    _locations.clear();
}

void UserLocationsModel::update() {
    beginResetModel();
    endResetModel();
}

void UserLocationsModel::deleteLocation(const QModelIndex& index) {
    UserLocation* location = _locations[index.row()];
    location->requestDelete();
}

void UserLocationsModel::renameLocation(const QModelIndex& index, const QString& newName) {
    UserLocation* location = _locations[index.row()];
    location->requestRename(newName);
}

void UserLocationsModel::refresh() {
    if (!_updating) {
        beginResetModel();
        qDeleteAll(_locations);
        _locations.clear();
        _updating = true;
        endResetModel();

        JSONCallbackParameters callbackParams(this, "handleLocationsResponse");
        AccountManager::getInstance().authenticatedRequest(LOCATIONS_GET,
                                                           QNetworkAccessManager::GetOperation,
                                                           callbackParams);
    }
}

void UserLocationsModel::handleLocationsResponse(const QJsonObject& responseData) {
    _updating = false;

    QJsonValue status = responseData["status"];
    if (!status.isUndefined() && status.toString() == "success") {
        beginResetModel();
        QJsonArray locations = responseData["data"].toObject()["locations"].toArray();
        for (QJsonArray::const_iterator it = locations.constBegin(); it != locations.constEnd(); it++) {
            QJsonObject location = (*it).toObject();
            QString locationAddress = "hifi://" + location["domain"].toObject()["name"].toString()
                + location["path"].toString();
            UserLocation* userLocation = new UserLocation(location["id"].toString(), location["name"].toString(),
                                                          locationAddress);
            _locations.append(userLocation);
            connect(userLocation, &UserLocation::deleted, this, &UserLocationsModel::removeLocation);
            connect(userLocation, &UserLocation::updated, this, &UserLocationsModel::update);
        }
        endResetModel();
    } else {
        qDebug() << "Error loading location data";
    }
}

void UserLocationsModel::removeLocation(const QString& name) {
    beginResetModel();
    for (QList<UserLocation*>::iterator it = _locations.begin(); it != _locations.end(); it++) {
        if ((*it)->name() == name) {
            _locations.erase(it);
            break;
        }
    }
    endResetModel();
}

int UserLocationsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }

    if (_updating) {
        return 1;
    }

    return _locations.length();
}

QVariant UserLocationsModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (_updating) {
            return QVariant("Updating...");
        } else if (index.row() > _locations.length()) {
            return QVariant();
        } else if (index.column() == NameColumn) {
            return _locations[index.row()]->name();
        } else if (index.column() == AddressColumn) {
            return QVariant(_locations[index.row()]->address());
        }
    }

    return QVariant();

}
QVariant UserLocationsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case NameColumn: return "Name";
            case AddressColumn: return "Address";
            default: return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags UserLocationsModel::flags(const QModelIndex& index) const {
    if (index.row() < _locations.length()) {
        UserLocation* ul = _locations[index.row()];
        if (ul->isUpdating()) {
            return Qt::NoItemFlags;
        }
    }

    return QAbstractListModel::flags(index);
}
