//
//  Tooltip.cpp
//  libraries/ui/src
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Tooltip.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QUuid>

#include <AccountManager.h>
#include <AddressManager.h>

HIFI_QML_DEF(Tooltip)

Tooltip::Tooltip(QQuickItem* parent) : QQuickItem(parent) {
    connect(this, &Tooltip::titleChanged, this, &Tooltip::requestHyperlinkImage);
}

Tooltip::~Tooltip() {

}

void Tooltip::setTitle(const QString& title) {
    if (title != _title) {
        _title = title;
        emit titleChanged();
    }
}

void Tooltip::setDescription(const QString& description) {
    if (description != _description) {
        _description = description;
        emit descriptionChanged();
    }
}

void Tooltip::setImageURL(const QString& imageURL) {
    if (imageURL != _imageURL) {
        _imageURL = imageURL;
        emit imageURLChanged();
    }
}

QString Tooltip::showTip(const QString& title, const QString& description) {
    const QString newTipId = QUuid().createUuid().toString();

    Tooltip::show([&](QQmlContext*, QObject* object) {
        object->setObjectName(newTipId);
        object->setProperty("title", title);
        object->setProperty("description", description);
    });

    return newTipId;
}

void Tooltip::closeTip(const QString& tipId) {
    auto rootItem = DependencyManager::get<OffscreenUi>()->getRootItem();
    QQuickItem* that = rootItem->findChild<QQuickItem*>(tipId);
    if (that) {
        that->deleteLater();
    }
}

void Tooltip::requestHyperlinkImage() {
    if (!_title.isEmpty()) {
        // we need to decide if this is a place name - if so we should ask the API for the associated image
        // and description (if we weren't given one via the entity properties)
        const QString PLACE_NAME_REGEX_STRING = "^[0-9A-Za-z](([0-9A-Za-z]|-(?!-))*[^\\W_]$|$)";

        QRegExp placeNameRegex(PLACE_NAME_REGEX_STRING);
        if (placeNameRegex.indexIn(_title) != -1) {
            // NOTE: I'm currently not 100% sure why the UI library needs networking, but it's linked for now
            // so I'm leveraging that here to get the place preview. We could also do this from the interface side
            // should the network link be removed from UI at a later date.

            // we possibly have a valid place name - so ask the API for the associated info
            auto accountManager = DependencyManager::get<AccountManager>();

            JSONCallbackParameters callbackParams;
            callbackParams.jsonCallbackReceiver = this;
            callbackParams.jsonCallbackMethod = "handleAPIResponse";

            accountManager->sendRequest(GET_PLACE.arg(_title),
                                        AccountManagerAuth::None,
                                        QNetworkAccessManager::GetOperation,
                                        callbackParams);
        }
    }
}

void Tooltip::handleAPIResponse(QNetworkReply& requestReply) {
    // did a preview image come back?
    QJsonObject responseObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    QJsonObject dataObject = responseObject["data"].toObject();

    const QString PLACE_KEY = "place";

    if (dataObject.contains(PLACE_KEY)) {
        QJsonObject placeObject = dataObject[PLACE_KEY].toObject();

        const QString PREVIEWS_KEY = "previews";
        const QString LOBBY_KEY = "lobby";

        if (placeObject.contains(PREVIEWS_KEY) && placeObject[PREVIEWS_KEY].toObject().contains(LOBBY_KEY)) {
            // we have previews - time to change the image URL
            setImageURL(placeObject[PREVIEWS_KEY].toObject()[LOBBY_KEY].toString());
        }

        if (_description.isEmpty()) {
            const QString DESCRIPTION_KEY = "description";
            // we have an empty description - did a non-empty description come back?
            if (placeObject.contains(DESCRIPTION_KEY)) {
                QString placeDescription = placeObject[DESCRIPTION_KEY].toString();

                if (!placeDescription.isEmpty()) {
                    // we got a non-empty description so change our description to that
                    setDescription(placeDescription);
                }
            }
        }
    }

}
