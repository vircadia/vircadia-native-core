//
//  BandwidthDialog.cpp
//  interface/src/ui
//
//  Created by Tobias Schwinger on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <cstdio>

#include "BandwidthRecorder.h"
#include "ui/BandwidthDialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>

#include <QPalette>
#include <QColor>


BandwidthChannelDisplay::BandwidthChannelDisplay(BandwidthRecorder::Channel *ch, QFormLayout* form,
                                                 char const* const caption, char const* unitCaption,
                                                 const float unitScale, unsigned colorRGBA) :
    caption(caption),
    unitCaption(unitCaption),
    unitScale(unitScale),
    colorRGBA(colorRGBA)
{
    this->ch = ch;

    label = new QLabel();
    label->setAlignment(Qt::AlignRight);

    QPalette palette = label->palette();
    unsigned rgb = colorRGBA >> 8;
    rgb = ((rgb & 0xfefefeu) >> 1) + ((rgb & 0xf8f8f8) >> 3);
    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
    label->setPalette(palette);

    form->addRow(QString(" ") + caption + " Bandwidth In/Out:", label);
}



void BandwidthChannelDisplay::bandwidthAverageUpdated() {
    strBuf =
        QString("").setNum((int) (ch->getAverageInputKilobitsPerSecond() * unitScale)) + "/" +
        QString("").setNum((int) (ch->getAverageOutputKilobitsPerSecond() * unitScale)) + " " + unitCaption;
}


void BandwidthChannelDisplay::Paint() {
    label->setText(strBuf);
}



BandwidthDialog::BandwidthDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) {

    this->setWindowTitle("Bandwidth Details");

    // Create layouter
    QFormLayout* form = new QFormLayout();
    this->QDialog::setLayout(form);

    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();

    _allChannelDisplays[0] = _audioChannelDisplay =
        new BandwidthChannelDisplay(&bandwidthRecorder->audioChannel, form, "Audio", "Kbps", 1.0, COLOR0);
    _allChannelDisplays[1] = _avatarsChannelDisplay =
        new BandwidthChannelDisplay(&bandwidthRecorder->avatarsChannel, form, "Avatars", "Kbps", 1.0, COLOR1);
    _allChannelDisplays[2] = _octreeChannelDisplay =
        new BandwidthChannelDisplay(&bandwidthRecorder->octreeChannel, form, "Octree", "Kbps", 1.0, COLOR2);
    _allChannelDisplays[3] = _metavoxelsChannelDisplay =
        new BandwidthChannelDisplay(&bandwidthRecorder->metavoxelsChannel, form, "Metavoxels", "Kbps", 1.0, COLOR2);
    _allChannelDisplays[4] = _otherChannelDisplay =
        new BandwidthChannelDisplay(&bandwidthRecorder->otherChannel, form, "Other", "Kbps", 1.0, COLOR2);
    _allChannelDisplays[5] = _totalChannelDisplay =
        new BandwidthChannelDisplay(&bandwidthRecorder->totalChannel, form, "Total", "Kbps", 1.0, COLOR2);
                          
    connect(averageUpdateTimer, SIGNAL(timeout()), this, SLOT(updateTimerTimeout()));
    averageUpdateTimer->start(1000);
}


BandwidthDialog::~BandwidthDialog() {
    for (unsigned int i=0; i<_CHANNELCOUNT; i++)
        delete _allChannelDisplays[i];
}


void BandwidthDialog::updateTimerTimeout() {
    for (unsigned int i=0; i<_CHANNELCOUNT; i++)
        _allChannelDisplays[i]->bandwidthAverageUpdated();
}


void BandwidthDialog::paintEvent(QPaintEvent* event) {
    for (unsigned int i=0; i<_CHANNELCOUNT; i++)
        _allChannelDisplays[i]->Paint();
    this->QDialog::paintEvent(event);
    this->setFixedSize(this->width(), this->height());
}


void BandwidthDialog::reject() {

    // Just regularly close upon ESC
    this->QDialog::close();
}


void BandwidthDialog::closeEvent(QCloseEvent* event) {

    this->QDialog::closeEvent(event);
    emit closed();
}

