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

#include <VoxelConstants.h>

#include "ui/LodToolsDialog.h"


LodToolsDialog::LodToolsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) 
{
    this->setWindowTitle("LOD Tools");

    // Create layouter
    QFormLayout* form = new QFormLayout();

    QSlider* lodSize = new QSlider(Qt::Horizontal);
    lodSize->setValue(0);
    form->addRow("LOD Size Scale:", lodSize);
    
    this->QDialog::setLayout(form);
}

LodToolsDialog::~LodToolsDialog() {
}

float LodToolsDialog::getVoxelSizeScale() {
    return DEFAULT_VOXEL_SIZE_SCALE;
}

void LodToolsDialog::paintEvent(QPaintEvent* event) {
    this->QDialog::paintEvent(event);
    this->setFixedSize(this->width(), this->height());
}

void LodToolsDialog::reject() {
    // Just regularly close upon ESC
    this->QDialog::close();
}

void LodToolsDialog::closeEvent(QCloseEvent* event) {
    this->QDialog::closeEvent(event);
    emit closed();
}


