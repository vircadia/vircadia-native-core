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
    
    foreach (const AnimationData& data, Application::getInstance()->getAvatar()->getAnimationData()) {
        addAnimation(data);
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

void AnimationsDialog::updateAnimationData() {
    QVector<AnimationData> data;
    for (int i = 0; i < _animations->count() - 1; i++) {
        data.append(static_cast<AnimationPanel*>(_animations->itemAt(i)->widget())->getAnimationData());
    }
    Application::getInstance()->getAvatar()->setAnimationData(data);
}

void AnimationsDialog::addAnimation(const AnimationData& data) {
    _animations->insertWidget(_animations->count() - 1, new AnimationPanel(this, data));
}

AnimationPanel::AnimationPanel(AnimationsDialog* dialog, const AnimationData& data) :
        _dialog(dialog),
        _applying(false) {
    setFrameStyle(QFrame::StyledPanel);
    
    QFormLayout* layout = new QFormLayout();
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    setLayout(layout);
    
    QHBoxLayout* urlBox = new QHBoxLayout();
    layout->addRow("URL:", urlBox);
    urlBox->addWidget(_url = new QLineEdit(data.url.toString()), 1);
    dialog->connect(_url, SIGNAL(returnPressed()), SLOT(updateAnimationData()));
    QPushButton* chooseURL = new QPushButton("Choose");
    urlBox->addWidget(chooseURL);
    connect(chooseURL, SIGNAL(clicked(bool)), SLOT(chooseURL()));
    
    layout->addRow("FPS:", _fps = new QDoubleSpinBox());
    _fps->setSingleStep(0.01);
    _fps->setMaximum(FLT_MAX);
    _fps->setValue(data.fps);
    dialog->connect(_fps, SIGNAL(valueChanged(double)), SLOT(updateAnimationData()));
    
    QPushButton* remove = new QPushButton("Delete");
    layout->addRow(remove);
    connect(remove, SIGNAL(clicked(bool)), SLOT(deleteLater()));
    dialog->connect(remove, SIGNAL(clicked(bool)), SLOT(updateAnimationData()), Qt::QueuedConnection);
}

AnimationData AnimationPanel::getAnimationData() const {
    AnimationData data;
    data.url = _url->text();
    data.fps = _fps->value();
    return data;
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

