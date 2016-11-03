//
//  Tooltip.h
//  libraries/ui/src
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Tooltip_h
#define hifi_Tooltip_h

#include <QtNetwork/QNetworkReply>

#include "OffscreenQmlDialog.h"

class Tooltip : public QQuickItem
{
    Q_OBJECT
    HIFI_QML_DECL

private:
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString imageURL READ getImageURL WRITE setImageURL NOTIFY imageURLChanged)

public:
    Tooltip(QQuickItem* parent = 0);
    virtual ~Tooltip();

    const QString& getTitle() const { return _title; }
    const QString& getDescription() const { return _description; }
    const QString& getImageURL() const { return _imageURL; }

    static QString showTip(const QString& title, const QString& description);
    static void closeTip(const QString& tipId);

public slots:
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setImageURL(const QString& imageURL);

signals:
    void titleChanged();
    void descriptionChanged();
    void imageURLChanged();

private slots:
    void handleAPIResponse(QNetworkReply& requestReply);

private:
    void requestHyperlinkImage();

    QString _title;
    QString _description;
    QString _imageURL { "../images/no-picture-provided.svg" };
};

#endif // hifi_Tooltip_h
