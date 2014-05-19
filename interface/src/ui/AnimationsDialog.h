//
//  AnimationsDialog.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 5/19/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimationsDialog_h
#define hifi_AnimationsDialog_h

#include <QDialog>
#include <QFrame>

#include "avatar/MyAvatar.h"

class QDoubleSpinner;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

/// Allows users to edit the avatar animations.
class AnimationsDialog : public QDialog {
    Q_OBJECT

public:
    
    AnimationsDialog();

    virtual void setVisible(bool visible);

public slots:
    
    void updateAnimationData();
    
private slots:
    
    void addAnimation(const AnimationData& animation = AnimationData());
    
private:
    
    QVBoxLayout* _animations;
    QPushButton* _ok;
};

/// A panel controlling a single animation.
class AnimationPanel : public QFrame {
    Q_OBJECT

public:
    
    AnimationPanel(AnimationsDialog* dialog, const AnimationData& data = AnimationData());

    AnimationData getAnimationData() const;

private slots:

    void chooseURL();
    
private:
    
    AnimationsDialog* _dialog;
    QLineEdit* _url;
    QDoubleSpinBox* _fps;
    bool _applying;
};

#endif // hifi_AnimationsDialog_h
