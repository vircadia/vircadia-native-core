//
//  AvatarAppearanceDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarAppearanceDialog_h
#define hifi_AvatarAppearanceDialog_h

#include "ui_avatarAppearance.h"

#include <QDialog>
#include <QString>

#include "scripting/WebWindowClass.h"

class AvatarAppearanceDialog : public QDialog {
    Q_OBJECT
    
public:
    AvatarAppearanceDialog(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* resizeEvent);

private:
    void loadAvatarAppearance();
    void saveAvatarAppearance();
    void openHeadModelBrowser();
    void openBodyModelBrowser();
    void openFullAvatarModelBrowser();
    void setUseFullAvatar(bool useFullAvatar);

    Ui_AvatarAppearanceDialog ui;

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
    void headURLChanged(const QString& newValue, const QString& modelName);
    void bodyURLChanged(const QString& newValue, const QString& modelName);
    void fullAvatarURLChanged(const QString& newValue, const QString& modelName);
    void useSeparateBodyAndHead(bool checked);
    void useFullAvatar(bool checked);
    
    
};

#endif // hifi_AvatarAppearanceDialog_h
