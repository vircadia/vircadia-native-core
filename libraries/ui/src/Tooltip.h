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
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    Tooltip(QQuickItem* parent = 0);
    virtual ~Tooltip();

    QString text() const;

    static QString showTip(const QString& text);
    static void closeTip(const QString& tipId);

public slots:
    virtual void setVisible(bool v);
    void setText(const QString& arg);

signals:
    void textChanged();

private:
    QString _text;
};

#endif // hifi_Tooltip_h
