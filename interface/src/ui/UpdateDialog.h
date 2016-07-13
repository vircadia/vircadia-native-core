//
//  UpdateDialog.h
//  interface/src/ui
//
//  Created by Leonardo Murillo on 6/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_UpdateDialog_h
#define hifi_UpdateDialog_h

#include <QtCore/QCoreApplication>

#include <OffscreenQmlDialog.h>

class UpdateDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL
    
    Q_PROPERTY(QString updateAvailableDetails READ updateAvailableDetails CONSTANT)
    Q_PROPERTY(QString releaseNotes READ releaseNotes CONSTANT)
    
public:
    UpdateDialog(QQuickItem* parent = nullptr);
    const QString& updateAvailableDetails() const;
    const QString& releaseNotes() const;

private:
    QString _updateAvailableDetails;
    QString _releaseNotes;

protected:
    Q_INVOKABLE void triggerUpgrade();

};

#endif // hifi_UpdateDialog_h
