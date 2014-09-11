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

class PreferencesDialog : public QDialog {
    Q_OBJECT
    
public:
    PreferencesDialog();

protected:
    void resizeEvent(QResizeEvent* resizeEvent);

private:
    void loadPreferences();
    void savePreferences();
    void openHeadModelBrowser();
    void openBodyModelBrowser();

    Ui_PreferencesDialog ui;
    QString _faceURLString;
    QString _skeletonURLString;
    QString _displayNameString;

private slots:
    void accept();
    void setHeadUrl(QString modelUrl);
    void setSkeletonUrl(QString modelUrl);
    void openSnapshotLocationBrowser();
    void openScriptsLocationBrowser();
    
};

#endif // hifi_PreferencesDialog_h
