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


BandwidthChannelDisplay::BandwidthChannelDisplay(BandwidthRecorder::Channel *ch, QFormLayout* form) {
    this->ch = ch;

    label = new QLabel();
    label->setAlignment(Qt::AlignRight);

    QPalette palette = label->palette();
    unsigned rgb = ch->colorRGBA >> 8;
    rgb = ((rgb & 0xfefefeu) >> 1) + ((rgb & 0xf8f8f8) >> 3);
    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
    label->setPalette(palette);

    form->addRow(QString(" ") + ch->caption + " Bandwidth In/Out:", label);
}



void BandwidthChannelDisplay::bandwidthAverageUpdated() {
    strBuf =
        QString("").setNum((int) (ch->getAverageInputKilobitsPerSecond() * ch->unitScale)) + "/" +
        QString("").setNum((int) (ch->getAverageOutputKilobitsPerSecond() * ch->unitScale)) + " " + ch->unitCaption;
}


void BandwidthChannelDisplay::Paint() {
    label->setText(strBuf);
}



BandwidthDialog::BandwidthDialog(QWidget* parent, BandwidthRecorder* model) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint),
    _model(model) {

    this->setWindowTitle("Bandwidth Details");

    // Create layouter
    QFormLayout* form = new QFormLayout();
    this->QDialog::setLayout(form);

    _allChannelDisplays[0] = _audioChannelDisplay = new BandwidthChannelDisplay(&_model->audioChannel, form);
    _allChannelDisplays[1] = _avatarsChannelDisplay = new BandwidthChannelDisplay(&_model->avatarsChannel, form);
    _allChannelDisplays[2] = _octreeChannelDisplay = new BandwidthChannelDisplay(&_model->octreeChannel, form);
    _allChannelDisplays[3] = _metavoxelsChannelDisplay = new BandwidthChannelDisplay(&_model->metavoxelsChannel, form);
    _allChannelDisplays[4] = _otherChannelDisplay = new BandwidthChannelDisplay(&_model->otherChannel, form);
    _allChannelDisplays[5] = _totalChannelDisplay = new BandwidthChannelDisplay(&_model->totalChannel, form);
                          
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

