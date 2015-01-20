//
//  LodToolsDialog.cpp
//  interface/src/ui
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCheckBox>
#include <QColor>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSlider>
#include <QString>

#include <LODManager.h>

#include "Menu.h"
#include "ui/LodToolsDialog.h"


LodToolsDialog::LodToolsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) 
{
    this->setWindowTitle("LOD Tools");
    auto lodManager = DependencyManager::get<LODManager>();

    // Create layouter
    QFormLayout* form = new QFormLayout(this);

    _lodSize = new QSlider(Qt::Horizontal, this);
    const int MAX_LOD_SIZE = MAX_LOD_SIZE_MULTIPLIER;
    const int MIN_LOD_SIZE = 0;
    const int STEP_LOD_SIZE = 1;
    const int PAGE_STEP_LOD_SIZE = 100;
    const int SLIDER_WIDTH = 300;
    _lodSize->setMaximum(MAX_LOD_SIZE);
    _lodSize->setMinimum(MIN_LOD_SIZE);
    _lodSize->setSingleStep(STEP_LOD_SIZE);
    _lodSize->setTickInterval(PAGE_STEP_LOD_SIZE);
    _lodSize->setTickPosition(QSlider::TicksBelow);
    _lodSize->setFixedWidth(SLIDER_WIDTH);
    _lodSize->setPageStep(PAGE_STEP_LOD_SIZE);
    int sliderValue = lodManager->getOctreeSizeScale() / TREE_SCALE;
    _lodSize->setValue(sliderValue);
    form->addRow("LOD Size Scale:", _lodSize);
    connect(_lodSize,SIGNAL(valueChanged(int)),this,SLOT(sizeScaleValueChanged(int)));

    _boundaryLevelAdjust = new QSlider(Qt::Horizontal, this);
    const int MAX_ADJUST = 10;
    const int MIN_ADJUST = 0;
    const int STEP_ADJUST = 1;
    _boundaryLevelAdjust->setMaximum(MAX_ADJUST);
    _boundaryLevelAdjust->setMinimum(MIN_ADJUST);
    _boundaryLevelAdjust->setSingleStep(STEP_ADJUST);
    _boundaryLevelAdjust->setTickInterval(STEP_ADJUST);
    _boundaryLevelAdjust->setTickPosition(QSlider::TicksBelow);
    _boundaryLevelAdjust->setFixedWidth(SLIDER_WIDTH);
    sliderValue = lodManager->getBoundaryLevelAdjust();
    _boundaryLevelAdjust->setValue(sliderValue);
    form->addRow("Boundary Level Adjust:", _boundaryLevelAdjust);
    connect(_boundaryLevelAdjust,SIGNAL(valueChanged(int)),this,SLOT(boundaryLevelValueChanged(int)));

    // Create a label with feedback...
    _feedback = new QLabel(this);
    QPalette palette = _feedback->palette();
    const unsigned redish  = 0xfff00000;
    palette.setColor(QPalette::WindowText, QColor::fromRgb(redish));
    _feedback->setPalette(palette);
    _feedback->setText(lodManager->getLODFeedbackText());
    const int FEEDBACK_WIDTH = 350;
    _feedback->setFixedWidth(FEEDBACK_WIDTH);
    form->addRow("You can see... ", _feedback);
    
    form->addRow("Automatic Avatar LOD Adjustment:", _automaticAvatarLOD = new QCheckBox(this));
    _automaticAvatarLOD->setChecked(lodManager->getAutomaticAvatarLOD());
    connect(_automaticAvatarLOD, SIGNAL(toggled(bool)), SLOT(updateAvatarLODControls()));
    
    form->addRow("Decrease Avatar LOD Below FPS:", _avatarLODDecreaseFPS = new QDoubleSpinBox(this));
    _avatarLODDecreaseFPS->setValue(lodManager->getAvatarLODDecreaseFPS());
    _avatarLODDecreaseFPS->setDecimals(0);
    connect(_avatarLODDecreaseFPS, SIGNAL(valueChanged(double)), SLOT(updateAvatarLODValues()));
    
    form->addRow("Increase Avatar LOD Above FPS:", _avatarLODIncreaseFPS = new QDoubleSpinBox(this));
    _avatarLODIncreaseFPS->setValue(lodManager->getAvatarLODIncreaseFPS());
    _avatarLODIncreaseFPS->setDecimals(0);
    connect(_avatarLODIncreaseFPS, SIGNAL(valueChanged(double)), SLOT(updateAvatarLODValues()));
    
    form->addRow("Avatar LOD:", _avatarLOD = new QDoubleSpinBox(this));
    _avatarLOD->setDecimals(3);
    _avatarLOD->setRange(1.0 / MAXIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER, 1.0 / MINIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER);
    _avatarLOD->setSingleStep(0.001);
    _avatarLOD->setValue(1.0 / lodManager->getAvatarLODDistanceMultiplier());
    connect(_avatarLOD, SIGNAL(valueChanged(double)), SLOT(updateAvatarLODValues()));
    
    // Add a button to reset
    QPushButton* resetButton = new QPushButton("Reset", this);
    form->addRow("", resetButton);
    connect(resetButton, SIGNAL(clicked(bool)), this, SLOT(resetClicked(bool)));

    this->QDialog::setLayout(form);
    
    updateAvatarLODControls();
}

void LodToolsDialog::reloadSliders() {
    auto lodManager = DependencyManager::get<LODManager>();
    _lodSize->setValue(lodManager->getOctreeSizeScale() / TREE_SCALE);
    _boundaryLevelAdjust->setValue(lodManager->getBoundaryLevelAdjust());
    _feedback->setText(lodManager->getLODFeedbackText());
}

void LodToolsDialog::updateAvatarLODControls() {
    QFormLayout* form = static_cast<QFormLayout*>(layout());
    
    auto lodManager = DependencyManager::get<LODManager>();
    lodManager->setAutomaticAvatarLOD(_automaticAvatarLOD->isChecked());
    
    _avatarLODDecreaseFPS->setVisible(_automaticAvatarLOD->isChecked());
    form->labelForField(_avatarLODDecreaseFPS)->setVisible(_automaticAvatarLOD->isChecked());
    
    _avatarLODIncreaseFPS->setVisible(_automaticAvatarLOD->isChecked());
    form->labelForField(_avatarLODIncreaseFPS)->setVisible(_automaticAvatarLOD->isChecked());
    
    _avatarLOD->setVisible(!_automaticAvatarLOD->isChecked());
    form->labelForField(_avatarLOD)->setVisible(!_automaticAvatarLOD->isChecked());
    
    if (!_automaticAvatarLOD->isChecked()) {
        _avatarLOD->setValue(1.0 / lodManager->getAvatarLODDistanceMultiplier());
    }
    
    if (isVisible()) {
        adjustSize();
    }
}

void LodToolsDialog::updateAvatarLODValues() {
    auto lodManager = DependencyManager::get<LODManager>();
    if (_automaticAvatarLOD->isChecked()) {
        lodManager->setAvatarLODDecreaseFPS(_avatarLODDecreaseFPS->value());
        lodManager->setAvatarLODIncreaseFPS(_avatarLODIncreaseFPS->value());
        
    } else {
        lodManager->setAvatarLODDistanceMultiplier(1.0 / _avatarLOD->value());
    }
}

void LodToolsDialog::sizeScaleValueChanged(int value) {
    auto lodManager = DependencyManager::get<LODManager>();
    float realValue = value * TREE_SCALE;
    lodManager->setOctreeSizeScale(realValue);
    
    _feedback->setText(lodManager->getLODFeedbackText());
}

void LodToolsDialog::boundaryLevelValueChanged(int value) {
    auto lodManager = DependencyManager::get<LODManager>();
    lodManager->setBoundaryLevelAdjust(value);
    _feedback->setText(lodManager->getLODFeedbackText());
}

void LodToolsDialog::resetClicked(bool checked) {
    int sliderValue = DEFAULT_OCTREE_SIZE_SCALE / TREE_SCALE;
    //sizeScaleValueChanged(sliderValue);
    _lodSize->setValue(sliderValue);
    _boundaryLevelAdjust->setValue(0);
    _automaticAvatarLOD->setChecked(true);
    _avatarLODDecreaseFPS->setValue(DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS);
    _avatarLODIncreaseFPS->setValue(ADJUST_LOD_UP_FPS);
}

void LodToolsDialog::reject() {
    // Just regularly close upon ESC
    this->QDialog::close();
}

void LodToolsDialog::closeEvent(QCloseEvent* event) {
    this->QDialog::closeEvent(event);
    emit closed();
}


