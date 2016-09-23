//
//  CachesSizeDialog.cpp
//
//
//  Created by Clement on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>

#include <AnimationCache.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <SoundCache.h>
#include <TextureCache.h>

#include "CachesSizeDialog.h"


QDoubleSpinBox* createDoubleSpinBox(QWidget* parent) {
    QDoubleSpinBox* box = new QDoubleSpinBox(parent);
    box->setDecimals(0);
    box->setRange(MIN_UNUSED_MAX_SIZE / BYTES_PER_MEGABYTES, MAX_UNUSED_MAX_SIZE / BYTES_PER_MEGABYTES);
    
    return box;
}

CachesSizeDialog::CachesSizeDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint)
{
    setWindowTitle("Caches Size");
    
    // Create layouter
    QFormLayout* form = new QFormLayout(this);
    setLayout(form);
    
    form->addRow("Animations cache size (MB):", _animations = createDoubleSpinBox(this));
    form->addRow("Geometries cache size (MB):", _geometries = createDoubleSpinBox(this));
    form->addRow("Sounds cache size (MB):", _sounds = createDoubleSpinBox(this));
    form->addRow("Textures cache size (MB):", _textures = createDoubleSpinBox(this));
    
    resetClicked(true);
    
    // Add a button to reset
    QPushButton* confirmButton = new QPushButton("Confirm", this);
    QPushButton* resetButton = new QPushButton("Reset", this);
    form->addRow(confirmButton, resetButton);
    connect(confirmButton, SIGNAL(clicked(bool)), this, SLOT(confirmClicked(bool)));
    connect(resetButton, SIGNAL(clicked(bool)), this, SLOT(resetClicked(bool)));
}

void CachesSizeDialog::confirmClicked(bool checked) {
    DependencyManager::get<AnimationCache>()->setUnusedResourceCacheSize(_animations->value() * BYTES_PER_MEGABYTES);
    DependencyManager::get<ModelCache>()->setUnusedResourceCacheSize(_geometries->value() * BYTES_PER_MEGABYTES);
    DependencyManager::get<SoundCache>()->setUnusedResourceCacheSize(_sounds->value() * BYTES_PER_MEGABYTES);
    // Disabling the texture cache because it's a liability in cases where we're overcommiting GPU memory
#if 0
    DependencyManager::get<TextureCache>()->setUnusedResourceCacheSize(_textures->value() * BYTES_PER_MEGABYTES);
#endif
    
    QDialog::close();
}

void CachesSizeDialog::resetClicked(bool checked) {
    _animations->setValue(DependencyManager::get<AnimationCache>()->getUnusedResourceCacheSize() / BYTES_PER_MEGABYTES);
    _geometries->setValue(DependencyManager::get<ModelCache>()->getUnusedResourceCacheSize() / BYTES_PER_MEGABYTES);
    _sounds->setValue(DependencyManager::get<SoundCache>()->getUnusedResourceCacheSize() / BYTES_PER_MEGABYTES);
    _textures->setValue(DependencyManager::get<TextureCache>()->getUnusedResourceCacheSize() / BYTES_PER_MEGABYTES);
}

void CachesSizeDialog::reject() {
    // Just regularly close upon ESC
    QDialog::close();
}

void CachesSizeDialog::closeEvent(QCloseEvent* event) {
    QDialog::closeEvent(event);
    emit closed();
}
