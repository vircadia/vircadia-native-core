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

#include "FramelessDialog.h"
#include "ui_preferencesDialog.h"

#include <QString>

class PreferencesDialog : public FramelessDialog {
    Q_OBJECT
    
public:
    PreferencesDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);

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
