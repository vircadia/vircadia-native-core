//
//  AnimationsDialog.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 5/19/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "AnimationsDialog.h"
#include "Application.h"

AnimationsDialog::AnimationsDialog() :
    QDialog(Application::getInstance()->getWindow()) {
    
    setWindowTitle("Edit Animations");
    setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);
    
    QScrollArea* area = new QScrollArea();
    layout->addWidget(area);
    area->setWidgetResizable(true);
    QWidget* container = new QWidget();
    container->setLayout(_animations = new QVBoxLayout());
    container->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    area->setWidget(container);
    _animations->addStretch(1);
    
    foreach (const AnimationHandlePointer& handle, Application::getInstance()->getAvatar()->getAnimationHandles()) {
        _animations->insertWidget(_animations->count() - 1, new AnimationPanel(this, handle));
    }
    
    QPushButton* newAnimation = new QPushButton("New Animation");
    connect(newAnimation, SIGNAL(clicked(bool)), SLOT(addAnimation()));
    layout->addWidget(newAnimation);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(buttons);
    connect(buttons, SIGNAL(accepted()), SLOT(deleteLater()));
    _ok = buttons->button(QDialogButtonBox::Ok);
    
    setMinimumSize(600, 600);
}

void AnimationsDialog::setVisible(bool visible) {
    QDialog::setVisible(visible);
    
    // un-default the OK button
    if (visible) {
        _ok->setDefault(false);
    }
}

void AnimationsDialog::addAnimation() {
    _animations->insertWidget(_animations->count() - 1, new AnimationPanel(
        this, Application::getInstance()->getAvatar()->addAnimationHandle()));
}

AnimationPanel::AnimationPanel(AnimationsDialog* dialog, const AnimationHandlePointer& handle) :
        _dialog(dialog),
        _handle(handle),
        _applying(false) {
    setFrameStyle(QFrame::StyledPanel);
    
    QFormLayout* layout = new QFormLayout();
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    setLayout(layout);
    
    layout->addRow("Role:", _role = new QComboBox());
    _role->addItem("idle");
    _role->addItem("sit");
    _role->setEditable(true);
    _role->setCurrentText(handle->getRole());
    connect(_role, SIGNAL(currentTextChanged(const QString&)), SLOT(updateHandle()));
    
    QHBoxLayout* urlBox = new QHBoxLayout();
    layout->addRow("URL:", urlBox);
    urlBox->addWidget(_url = new QLineEdit(handle->getURL().toString()), 1);
    connect(_url, SIGNAL(returnPressed()), SLOT(updateHandle()));
    QPushButton* chooseURL = new QPushButton("Choose");
    urlBox->addWidget(chooseURL);
    connect(chooseURL, SIGNAL(clicked(bool)), SLOT(chooseURL()));
    
    layout->addRow("FPS:", _fps = new QDoubleSpinBox());
    _fps->setSingleStep(0.01);
    _fps->setMinimum(-FLT_MAX);
    _fps->setMaximum(FLT_MAX);
    _fps->setValue(handle->getFPS());
    connect(_fps, SIGNAL(valueChanged(double)), SLOT(updateHandle()));
    
    layout->addRow("Priority:", _priority = new QDoubleSpinBox());
    _priority->setSingleStep(0.01);
    _priority->setMinimum(-FLT_MAX);
    _priority->setMaximum(FLT_MAX);
    _priority->setValue(handle->getPriority());
    connect(_priority, SIGNAL(valueChanged(double)), SLOT(updateHandle()));
    
    QHBoxLayout* maskedJointBox = new QHBoxLayout();
    layout->addRow("Masked Joints:", maskedJointBox);
    maskedJointBox->addWidget(_maskedJoints = new QLineEdit(handle->getMaskedJoints().join(", ")), 1);
    connect(_maskedJoints, SIGNAL(returnPressed()), SLOT(updateHandle()));
    maskedJointBox->addWidget(_chooseMaskedJoints = new QPushButton("Choose"));
    connect(_chooseMaskedJoints, SIGNAL(clicked(bool)), SLOT(chooseMaskedJoints()));
    
    layout->addRow("Loop:", _loop = new QCheckBox());
    _loop->setChecked(handle->getLoop());
    connect(_loop, SIGNAL(toggled(bool)), SLOT(updateHandle()));
    
    layout->addRow("Hold:", _hold = new QCheckBox());
    _hold->setChecked(handle->getHold());
    connect(_hold, SIGNAL(toggled(bool)), SLOT(updateHandle()));
    
    layout->addRow("Start Automatically:", _startAutomatically = new QCheckBox());
    _startAutomatically->setChecked(handle->getStartAutomatically());
    connect(_startAutomatically, SIGNAL(toggled(bool)), SLOT(updateHandle()));
    
    layout->addRow("First Frame:", _firstFrame = new QDoubleSpinBox());
    _firstFrame->setSingleStep(0.01);
    _firstFrame->setMaximum(INT_MAX);
    _firstFrame->setValue(handle->getFirstFrame());
    connect(_firstFrame, SIGNAL(valueChanged(double)), SLOT(updateHandle()));
    
    layout->addRow("Last Frame:", _lastFrame = new QDoubleSpinBox());
    _lastFrame->setSingleStep(0.01);
    _lastFrame->setMaximum(INT_MAX);
    _lastFrame->setValue(handle->getLastFrame());
    connect(_lastFrame, SIGNAL(valueChanged(double)), SLOT(updateHandle()));
    
    QHBoxLayout* buttons = new QHBoxLayout();
    layout->addRow(buttons);
    buttons->addWidget(_start = new QPushButton("Start"));
    _handle->connect(_start, SIGNAL(clicked(bool)), SLOT(start())); 
    buttons->addWidget(_stop = new QPushButton("Stop"));
    _handle->connect(_stop, SIGNAL(clicked(bool)), SLOT(stop()));
    QPushButton* remove = new QPushButton("Delete");
    buttons->addWidget(remove);
    connect(remove, SIGNAL(clicked(bool)), SLOT(removeHandle()));
    
    _stop->connect(_handle.data(), SIGNAL(runningChanged(bool)), SLOT(setEnabled(bool)));
    _stop->setEnabled(_handle->isRunning());
}

void AnimationPanel::chooseURL() {
    QString directory = Application::getInstance()->lockSettings()->value("animation_directory").toString();
    Application::getInstance()->unlockSettings();
    QString filename = QFileDialog::getOpenFileName(this, "Choose Animation", directory, "Animation files (*.fbx)");
    if (filename.isEmpty()) {
        return;
    }
    Application::getInstance()->lockSettings()->setValue("animation_directory", QFileInfo(filename).path());
    Application::getInstance()->unlockSettings();
    _url->setText(QUrl::fromLocalFile(filename).toString());
    emit _url->returnPressed();
}

void AnimationPanel::chooseMaskedJoints() {
    QMenu menu;
    QStringList maskedJoints = _handle->getMaskedJoints();
    foreach (const QString& jointName, Application::getInstance()->getAvatar()->getJointNames()) {
        QAction* action = menu.addAction(jointName);
        action->setCheckable(true);
        action->setChecked(maskedJoints.contains(jointName));
    }
    QAction* action = menu.exec(_chooseMaskedJoints->mapToGlobal(QPoint(0, 0)));
    if (action) {
        if (action->isChecked()) {
            maskedJoints.append(action->text());
        } else {
            maskedJoints.removeOne(action->text());
        }
        _handle->setMaskedJoints(maskedJoints);
        _maskedJoints->setText(maskedJoints.join(", "));
    }
}

void AnimationPanel::updateHandle() {
    _handle->setRole(_role->currentText());
    _handle->setURL(_url->text());
    _handle->setFPS(_fps->value());
    _handle->setPriority(_priority->value());
    _handle->setLoop(_loop->isChecked());
    _handle->setHold(_hold->isChecked());
    _handle->setStartAutomatically(_startAutomatically->isChecked());
    _handle->setFirstFrame(_firstFrame->value());
    _handle->setLastFrame(_lastFrame->value());
    _handle->setMaskedJoints(_maskedJoints->text().split(QRegExp("\\s*,\\s*")));
}

void AnimationPanel::removeHandle() {
    Application::getInstance()->getAvatar()->removeAnimationHandle(_handle);
    deleteLater();
}
