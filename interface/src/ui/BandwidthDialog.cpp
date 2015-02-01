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

#include <cstdio>

#include "BandwidthRecorder.h"
#include "ui/BandwidthDialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>

#include <QPalette>
#include <QColor>


BandwidthDialog::ChannelDisplay::ChannelDisplay(BandwidthRecorder::Channel *ch, QFormLayout* form) {
    this->ch = ch;
    this->label = setupLabel(form);
}


QLabel* BandwidthDialog::ChannelDisplay::setupLabel(QFormLayout* form) {
    QLabel* label = new QLabel();

    label->setAlignment(Qt::AlignRight);

    QPalette palette = label->palette();
    unsigned rgb = ch->colorRGBA >> 8;
    rgb = ((rgb & 0xfefefeu) >> 1) + ((rgb & 0xf8f8f8) >> 3);
    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
    label->setPalette(palette);

    form->addRow((std::string(" ") + ch->caption + " Bandwidth In/Out:").c_str(), label);

    return label;
}



void BandwidthDialog::ChannelDisplay::setLabelText() {
    std::string strBuf =
        std::to_string ((int) (ch->getAverageInputKilobitsPerSecond() * ch->unitScale)) + "/" +
        std::to_string ((int) (ch->getAverageOutputKilobitsPerSecond() * ch->unitScale)) + " " + ch->unitCaption;
    label->setText(strBuf.c_str());
}



BandwidthDialog::BandwidthDialog(QWidget* parent, BandwidthRecorder* model) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint),
    _model(model) {

    this->setWindowTitle("Bandwidth Details");

    // Create layouter
    QFormLayout* form = new QFormLayout();
    this->QDialog::setLayout(form);

    audioChannelDisplay = new ChannelDisplay(_model->audioChannel, form);
    avatarsChannelDisplay = new ChannelDisplay(_model->avatarsChannel, form);
    octreeChannelDisplay = new ChannelDisplay(_model->octreeChannel, form);
    metavoxelsChannelDisplay = new ChannelDisplay(_model->metavoxelsChannel, form);
    otherChannelDisplay = new ChannelDisplay(_model->otherChannel, form);
    totalChannelDisplay = new ChannelDisplay(_model->totalChannel, form);
}


BandwidthDialog::~BandwidthDialog() {
    delete audioChannelDisplay;
    delete avatarsChannelDisplay;
    delete octreeChannelDisplay;
    delete metavoxelsChannelDisplay;
    delete otherChannelDisplay;
    delete totalChannelDisplay;
}


void BandwidthDialog::paintEvent(QPaintEvent* event) {
    audioChannelDisplay->setLabelText();
    avatarsChannelDisplay->setLabelText();
    octreeChannelDisplay->setLabelText();
    metavoxelsChannelDisplay->setLabelText();
    otherChannelDisplay->setLabelText();
    totalChannelDisplay->setLabelText();

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

