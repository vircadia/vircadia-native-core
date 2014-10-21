//
//  MetavoxelNetworkSimulator.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 10/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>

#include "Application.h"
#include "MetavoxelNetworkSimulator.h"

MetavoxelNetworkSimulator::MetavoxelNetworkSimulator() :
    QWidget(Application::getInstance()->getGLWidget(), Qt::Dialog) {
    
    setWindowTitle("Metavoxel Network Simulator");
    setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* topLayout = new QVBoxLayout();
    setLayout(topLayout);
    
    QFormLayout* form = new QFormLayout();
    topLayout->addLayout(form);
    
    MetavoxelSystem::NetworkSimulation simulation = Application::getInstance()->getMetavoxels()->getNetworkSimulation();
    
    form->addRow("Drop Rate:", _dropRate = new QDoubleSpinBox());
    _dropRate->setSuffix("%");
    _dropRate->setValue(simulation.dropRate * 100.0);
    connect(_dropRate, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
        &MetavoxelNetworkSimulator::updateMetavoxelSystem);

    form->addRow("Repeat Rate:", _repeatRate = new QDoubleSpinBox());
    _repeatRate->setSuffix("%");
    _repeatRate->setValue(simulation.repeatRate * 100.0);
    connect(_repeatRate, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
        &MetavoxelNetworkSimulator::updateMetavoxelSystem);
    
    form->addRow("Minimum Delay:", _minimumDelay = new QSpinBox());
    _minimumDelay->setMaximum(1000);
    _minimumDelay->setSuffix("ms");
    _minimumDelay->setValue(simulation.minimumDelay);
    connect(_minimumDelay, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        &MetavoxelNetworkSimulator::updateMetavoxelSystem);
    
    form->addRow("Maximum Delay:", _maximumDelay = new QSpinBox());
    _maximumDelay->setMaximum(1000);
    _maximumDelay->setSuffix("ms");
    _maximumDelay->setValue(simulation.maximumDelay);
    connect(_maximumDelay, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        &MetavoxelNetworkSimulator::updateMetavoxelSystem);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    topLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QWidget::close);
    
    show();
}

void MetavoxelNetworkSimulator::updateMetavoxelSystem() {
    Application::getInstance()->getMetavoxels()->setNetworkSimulation(MetavoxelSystem::NetworkSimulation(
        _dropRate->value() / 100.0, _repeatRate->value() / 100.0, qMin(_minimumDelay->value(), _maximumDelay->value()),
            qMax(_minimumDelay->value(), _maximumDelay->value())));
}
