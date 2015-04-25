//
//  OffscreenQmlDialog.h
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_OffscreenQmlDialog_h
#define hifi_OffscreenQmlDialog_h

#include <QQuickItem>

#include "OffscreenUi.h"

class OffscreenQmlDialog : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_ENUMS(StandardButton)
    Q_FLAGS(StandardButtons)

public:
    OffscreenQmlDialog(QQuickItem* parent = nullptr);
    virtual ~OffscreenQmlDialog();

    enum StandardButton {
        // keep this in sync with QDialogButtonBox::StandardButton and QMessageBox::StandardButton
        NoButton = 0x00000000,
        Ok = 0x00000400,
        Save = 0x00000800,
        SaveAll = 0x00001000,
        Open = 0x00002000,
        Yes = 0x00004000,
        YesToAll = 0x00008000,
        No = 0x00010000,
        NoToAll = 0x00020000,
        Abort = 0x00040000,
        Retry = 0x00080000,
        Ignore = 0x00100000,
        Close = 0x00200000,
        Cancel = 0x00400000,
        Discard = 0x00800000,
        Help = 0x01000000,
        Apply = 0x02000000,
        Reset = 0x04000000,
        RestoreDefaults = 0x08000000,
        NButtons
    };
    Q_DECLARE_FLAGS(StandardButtons, StandardButton)

protected:
    void hide();
    virtual void accept();
    virtual void reject();

public:    
    QString title() const;
    void setTitle(const QString& title);

signals:
    void accepted();
    void rejected();
    void titleChanged();

private:
    QString _title;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(OffscreenQmlDialog::StandardButtons)

#endif
