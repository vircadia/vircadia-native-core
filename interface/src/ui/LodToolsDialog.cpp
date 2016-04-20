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

    form->addRow("Manually Adjust Level of Detail:", _manualLODAdjust = new QCheckBox(this));
    _manualLODAdjust->setChecked(!lodManager->getAutomaticLODAdjust());
    connect(_manualLODAdjust, SIGNAL(toggled(bool)), SLOT(updateAutomaticLODAdjust()));
    
    _lodSize = new QSlider(Qt::Horizontal, this);
    const int MAX_LOD_SIZE = 2000; // ~20:4 vision -- really good.
    const int MIN_LOD_SIZE = 5; // ~20:1600 vision -- really bad!
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
    form->addRow("Level of Detail:", _lodSize);
    connect(_lodSize,SIGNAL(valueChanged(int)),this,SLOT(sizeScaleValueChanged(int)));
    
    // Add a button to reset
    QPushButton* resetButton = new QPushButton("Reset", this);
    form->addRow("", resetButton);
    connect(resetButton, SIGNAL(clicked(bool)), this, SLOT(resetClicked(bool)));

    this->QDialog::setLayout(form);
    
    updateAutomaticLODAdjust();
}

void LodToolsDialog::reloadSliders() {
    auto lodManager = DependencyManager::get<LODManager>();
    _lodSize->setValue(lodManager->getOctreeSizeScale() / TREE_SCALE);
    _feedback->setText(lodManager->getLODFeedbackText());
}

void LodToolsDialog::updateAutomaticLODAdjust() {
    auto lodManager = DependencyManager::get<LODManager>();
    lodManager->setAutomaticLODAdjust(!_manualLODAdjust->isChecked());
    _lodSize->setEnabled(_manualLODAdjust->isChecked());
}

void LodToolsDialog::sizeScaleValueChanged(int value) {
    auto lodManager = DependencyManager::get<LODManager>();
    float realValue = value * TREE_SCALE;
    lodManager->setOctreeSizeScale(realValue);
    
    _feedback->setText(lodManager->getLODFeedbackText());
}

void LodToolsDialog::resetClicked(bool checked) {

    int sliderValue = DEFAULT_OCTREE_SIZE_SCALE / TREE_SCALE;
    _lodSize->setValue(sliderValue);
    _manualLODAdjust->setChecked(false);
    
    updateAutomaticLODAdjust(); // tell our LOD manager about the reset
}

void LodToolsDialog::reject() {
    // Just regularly close upon ESC
    this->QDialog::close();
}

void LodToolsDialog::closeEvent(QCloseEvent* event) {
    this->QDialog::closeEvent(event);
    emit closed();
    
#if RESET_TO_AUTOMATIC_WHEN_YOU_CLOSE_THE_DIALOG_BOX
    auto lodManager = DependencyManager::get<LODManager>();
    
    // always revert back to automatic LOD adjustment when closed
    lodManager->setAutomaticLODAdjust(true); 

    // if the user adjusted the LOD above "normal" then always revert back to default
    if (lodManager->getOctreeSizeScale() > DEFAULT_OCTREE_SIZE_SCALE) {
        lodManager->setOctreeSizeScale(DEFAULT_OCTREE_SIZE_SCALE);
    }
#endif
}


