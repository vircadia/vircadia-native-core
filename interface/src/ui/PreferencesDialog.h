//
//  PreferencesDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PreferencesDialog_h
#define hifi_PreferencesDialog_h

#include "ui_preferencesDialog.h"

#include <QDialog>
#include <QString>

class WebWindowClass;

class PreferencesDialog : public QDialog {
    Q_OBJECT
    
public:
    PreferencesDialog(QWidget* parent = nullptr);

    void avatarDescriptionChanged();

protected:
    void resizeEvent(QResizeEvent* resizeEvent);

private:
    void loadPreferences();
    void savePreferences();
    QUrl _lastGoodAvatarURL;
    QString _lastGoodAvatarName;
    void restoreLastGoodAvatar();

    Ui_PreferencesDialog ui;

    QString _displayNameString;
    
    WebWindowClass* _marketplaceWindow { nullptr };

private slots:
    void accept();
    void reject();
    void openFullAvatarModelBrowser();
    void openSnapshotLocationBrowser();
    void openScriptsLocationBrowser();
    void changeFullAvatarURL();
    void fullAvatarURLChanged(const QString& newValue, const QString& modelName);
    void changeUseAcuity();
};

#endif // hifi_PreferencesDialog_h
