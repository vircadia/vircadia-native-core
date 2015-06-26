//
//  Tooltip.h
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

#include "OffscreenQmlDialog.h"

class Tooltip : public QQuickItem
{
    Q_OBJECT
    HIFI_QML_DECL

private:
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription NOTIFY descriptionChanged)

public:
    Tooltip(QQuickItem* parent = 0);
    virtual ~Tooltip();

    const QString& getTitle() const;
    const QString& getDescription() const;

    static QString showTip(const QString& title, const QString& description);
    static void closeTip(const QString& tipId);

public slots:
    virtual void setVisible(bool v);
    void setTitle(const QString& title);
    void setDescription(const QString& description);

signals:
    void titleChanged();
    void descriptionChanged();

private:
    QString _title;
    QString _description;
};

#endif // hifi_Tooltip_h
