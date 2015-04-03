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

#include "scripting/WebWindowClass.h"

class PreferencesDialog : public QDialog {
    Q_OBJECT
    
public:
    PreferencesDialog(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* resizeEvent);

private:
    void loadPreferences();
    void savePreferences();
    void openHeadModelBrowser();
    void openBodyModelBrowser();
    void openFullAvatarModelBrowser();
    void setUseFullAvatar(bool useFullAvatar);

    Ui_PreferencesDialog ui;

    bool _useFullAvatar;
    QString _headURLString;
    QString _bodyURLString;
    QString _fullAvatarURLString;
    
    
    QString _displayNameString;
    
    WebWindowClass* _marketplaceWindow = NULL;

private slots:
    void accept();
    void setHeadUrl(QString modelUrl);
    void setSkeletonUrl(QString modelUrl);
    void openSnapshotLocationBrowser();
    void openScriptsLocationBrowser();
    void headURLChanged(const QString& newValue);
    void bodyURLChanged(const QString& newValue);
    void fullAvatarURLChanged(const QString& newValue);
    void useSeparateBodyAndHead(bool checked);
    void useFullAvatar(bool checked);
    
};

#endif // hifi_PreferencesDialog_h
