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
#include <QDoubleSpinBox>
#include <QFrame>

#include <SettingHandle.h>

#include "avatar/MyAvatar.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinner;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

/// Allows users to edit the avatar animations.
class AnimationsDialog : public QDialog {
    Q_OBJECT

public:
    
    AnimationsDialog(QWidget* parent = nullptr);

    virtual void setVisible(bool visible);

private slots:

    void addAnimation();

private:
    
    QVBoxLayout* _animations = nullptr;
    QPushButton* _ok = nullptr;
};

/// A panel controlling a single animation.
class AnimationPanel : public QFrame {
    Q_OBJECT

public:
    
    AnimationPanel(AnimationsDialog* dialog, const AnimationHandlePointer& handle);

private slots:

    void chooseURL();
    void chooseMaskedJoints();
    void updateHandle();
    void removeHandle();
    
private:
    
    AnimationsDialog* _dialog = nullptr;
    AnimationHandlePointer _handle;
    QComboBox* _role = nullptr;
    QLineEdit* _url = nullptr;
    QDoubleSpinBox* _fps = nullptr;
    QDoubleSpinBox* _priority = nullptr;
    QCheckBox* _loop = nullptr;
    QCheckBox* _hold = nullptr;
    QCheckBox* _startAutomatically = nullptr;
    QDoubleSpinBox* _firstFrame = nullptr;
    QDoubleSpinBox* _lastFrame = nullptr;
    QLineEdit* _maskedJoints = nullptr;
    QPushButton* _chooseMaskedJoints = nullptr;
    QPushButton* _start = nullptr;
    QPushButton* _stop = nullptr;
    bool _applying;
    
    static Setting::Handle<QString> _animationDirectory;
};

#endif // hifi_AnimationsDialog_h
