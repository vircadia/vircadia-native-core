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
    
    QHBoxLayout* urlBox = new QHBoxLayout();
    layout->addRow("URL:", urlBox);
    urlBox->addWidget(_url = new QLineEdit(handle->getURL().toString()), 1);
    connect(_url, SIGNAL(returnPressed()), SLOT(updateHandle()));
    QPushButton* chooseURL = new QPushButton("Choose");
    urlBox->addWidget(chooseURL);
    connect(chooseURL, SIGNAL(clicked(bool)), SLOT(chooseURL()));
    
    layout->addRow("FPS:", _fps = new QDoubleSpinBox());
    _fps->setSingleStep(0.01);
    _fps->setMaximum(FLT_MAX);
    _fps->setValue(handle->getFPS());
    connect(_fps, SIGNAL(valueChanged(double)), SLOT(updateHandle()));
    
    layout->addRow("Priority:", _priority = new QDoubleSpinBox());
    _priority->setSingleStep(0.01);
    _priority->setMinimum(-FLT_MAX);
    _priority->setMaximum(FLT_MAX);
    _priority->setValue(handle->getPriority());
    connect(_priority, SIGNAL(valueChanged(double)), SLOT(updateHandle()));
    
    QPushButton* remove = new QPushButton("Delete");
    layout->addRow(remove);
    connect(remove, SIGNAL(clicked(bool)), SLOT(removeHandle()));
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

void AnimationPanel::updateHandle() {
    _handle->setURL(_url->text());
    _handle->setFPS(_fps->value());
    _handle->setPriority(_priority->value());
}

void AnimationPanel::removeHandle() {
    Application::getInstance()->getAvatar()->removeAnimationHandle(_handle);
    deleteLater();
}
