//
//  ErrorDialog.h
//
//  Created by David Rowe on 30 May 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_ErrorDialog_h
#define hifi_ErrorDialog_h

#include "OffscreenQmlDialog.h"

class ErrorDialog : public OffscreenQmlDialog
{
    Q_OBJECT
    HIFI_QML_DECL

private:
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    ErrorDialog(QQuickItem* parent = 0);
    virtual ~ErrorDialog();

    QString text() const;

public slots:
    void setText(const QString& arg);

signals:
    void textChanged();

protected slots:
    virtual void accept() override;

private:
    QString _text;
};

#endif // hifi_ErrorDialog_h
