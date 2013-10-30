//
//  LodToolsDialog.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QFormLayout>
#include <QDialogButtonBox>

#include <QPalette>
#include <QColor>
#include <QSlider>
#include <QPushButton>
#include <QString>

#include <VoxelConstants.h>

#include "Menu.h"
#include "ui/LodToolsDialog.h"


LodToolsDialog::LodToolsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) 
{
    this->setWindowTitle("LOD Tools");

    // Create layouter
    QFormLayout* form = new QFormLayout();

    _lodSize = new QSlider(Qt::Horizontal);
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
    int sliderValue = Menu::getInstance()->getVoxelSizeScale() / TREE_SCALE;
    _lodSize->setValue(sliderValue);
    form->addRow("LOD Size Scale:", _lodSize);
    connect(_lodSize,SIGNAL(valueChanged(int)),this,SLOT(sizeScaleValueChanged(int)));

    _boundaryLevelAdjust = new QSlider(Qt::Horizontal);
    const int MAX_ADJUST = 10;
    const int MIN_ADJUST = 0;
    const int STEP_ADJUST = 1;
    _boundaryLevelAdjust->setMaximum(MAX_ADJUST);
    _boundaryLevelAdjust->setMinimum(MIN_ADJUST);
    _boundaryLevelAdjust->setSingleStep(STEP_ADJUST);
    _boundaryLevelAdjust->setTickInterval(STEP_ADJUST);
    _boundaryLevelAdjust->setTickPosition(QSlider::TicksBelow);
    _boundaryLevelAdjust->setFixedWidth(SLIDER_WIDTH);
    sliderValue = Menu::getInstance()->getBoundaryLevelAdjust();
    _boundaryLevelAdjust->setValue(sliderValue);
    form->addRow("Boundary Level Adjust:", _boundaryLevelAdjust);
    connect(_boundaryLevelAdjust,SIGNAL(valueChanged(int)),this,SLOT(boundaryLevelValueChanged(int)));

    // Create a label with feedback...
    _feedback = new QLabel();
    QPalette palette = _feedback->palette();
    const unsigned redish  = 0xfff00000;
    palette.setColor(QPalette::WindowText, QColor::fromRgb(redish));
    _feedback->setPalette(palette);
    _feedback->setText(getFeedbackText());
    const int FEEDBACK_WIDTH = 350;
    _feedback->setFixedWidth(FEEDBACK_WIDTH);
    form->addRow("You can see... ", _feedback);
    
    // Add a button to reset
    QPushButton* resetButton = new QPushButton("Reset");
    form->addRow("", resetButton);
    connect(resetButton,SIGNAL(clicked(bool)),this,SLOT(resetClicked(bool)));

    this->QDialog::setLayout(form);
}

QString LodToolsDialog::getFeedbackText() {
    // determine granularity feedback
    int boundaryLevelAdjust = Menu::getInstance()->getBoundaryLevelAdjust();
    QString granularityFeedback;

    switch (boundaryLevelAdjust) {
        case 0: {
            granularityFeedback = QString("at standard granularity.");
        } break;
        case 1: {
            granularityFeedback = QString("at half of standard granularity.");
        } break;
        case 2: {
            granularityFeedback = QString("at a third of standard granularity.");
        } break;
        default: {
            granularityFeedback = QString("at 1/%1th of standard granularity.").arg(boundaryLevelAdjust + 1);
        } break;
    }

    // distance feedback    
    float voxelSizeScale = Menu::getInstance()->getVoxelSizeScale();
    float relativeToDefault = voxelSizeScale / DEFAULT_VOXEL_SIZE_SCALE;
    QString result;
    if (relativeToDefault > 1.01) {
        result = QString("%1 further %2").arg(relativeToDefault,8,'f',2).arg(granularityFeedback);
    } else if (relativeToDefault > 0.99) {
            result = QString("the default distance %1").arg(granularityFeedback);
    } else {
        result = QString("%1 of default %2").arg(relativeToDefault,8,'f',3).arg(granularityFeedback);
    }
    return result;
}

LodToolsDialog::~LodToolsDialog() {
    delete _feedback;
    delete _lodSize;
    delete _boundaryLevelAdjust;
}

void LodToolsDialog::sizeScaleValueChanged(int value) {
    float realValue = value * TREE_SCALE;
    Menu::getInstance()->setVoxelSizeScale(realValue);
    
    _feedback->setText(getFeedbackText());
}

void LodToolsDialog::boundaryLevelValueChanged(int value) {
    Menu::getInstance()->setBoundaryLevelAdjust(value);
    _feedback->setText(getFeedbackText());
}

void LodToolsDialog::resetClicked(bool checked) {
    int sliderValue = DEFAULT_VOXEL_SIZE_SCALE / TREE_SCALE;
    //sizeScaleValueChanged(sliderValue);
    _lodSize->setValue(sliderValue);
    _boundaryLevelAdjust->setValue(0);
}

void LodToolsDialog::reject() {
    // Just regularly close upon ESC
    this->QDialog::close();
}

void LodToolsDialog::closeEvent(QCloseEvent* event) {
    this->QDialog::closeEvent(event);
    emit closed();
}


