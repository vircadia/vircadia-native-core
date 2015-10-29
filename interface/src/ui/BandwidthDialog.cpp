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


BandwidthChannelDisplay::BandwidthChannelDisplay(QVector<NodeType_t> nodeTypesToFollow,
                                                 QFormLayout* form,
                                                 char const* const caption, char const* unitCaption,
                                                 const float unitScale, unsigned colorRGBA) :
    _nodeTypesToFollow(nodeTypesToFollow),
    _caption(caption),
    _unitCaption(unitCaption),
    _unitScale(unitScale),
    _colorRGBA(colorRGBA)
{
    _label = new QLabel();
    _label->setAlignment(Qt::AlignRight);

    QPalette palette = _label->palette();
    unsigned rgb = colorRGBA >> 8;
    rgb = ((rgb & 0xfefefeu) >> 1) + ((rgb & 0xf8f8f8) >> 3);
    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
    _label->setPalette(palette);

    form->addRow(QString(" ") + _caption + " Bandwidth In/Out:", _label);
}



void BandwidthChannelDisplay::bandwidthAverageUpdated() {
    float inTotal = 0.;
    float outTotal = 0.;

    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();

    for (int i = 0; i < _nodeTypesToFollow.size(); ++i) {
        inTotal += bandwidthRecorder->getAverageInputKilobitsPerSecond(_nodeTypesToFollow.at(i));
        outTotal += bandwidthRecorder->getAverageOutputKilobitsPerSecond(_nodeTypesToFollow.at(i));
    }

    _strBuf =
        QString("").setNum((int) (inTotal * _unitScale)) + "/" +
        QString("").setNum((int) (outTotal * _unitScale)) + " " + _unitCaption;
}


void BandwidthChannelDisplay::paint() {
    _label->setText(_strBuf);
}


BandwidthDialog::BandwidthDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) {

    this->setWindowTitle("Bandwidth Details");

    // Create layout
    QFormLayout* form = new QFormLayout();
    form->setSizeConstraint(QLayout::SetFixedSize);
    this->QDialog::setLayout(form);

    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();

    _allChannelDisplays[0] = _audioChannelDisplay =
        new BandwidthChannelDisplay({NodeType::AudioMixer}, form, "Audio", "Kbps", 1.0, COLOR0);
    _allChannelDisplays[1] = _avatarsChannelDisplay =
        new BandwidthChannelDisplay({NodeType::Agent, NodeType::AvatarMixer}, form, "Avatars", "Kbps", 1.0, COLOR1);
    _allChannelDisplays[2] = _octreeChannelDisplay =
        new BandwidthChannelDisplay({NodeType::EntityServer}, form, "Octree", "Kbps", 1.0, COLOR2);
    _allChannelDisplays[3] = _octreeChannelDisplay =
        new BandwidthChannelDisplay({NodeType::DomainServer}, form, "Domain", "Kbps", 1.0, COLOR2);
    _allChannelDisplays[4] = _otherChannelDisplay =
        new BandwidthChannelDisplay({NodeType::Unassigned}, form, "Other", "Kbps", 1.0, COLOR2);
    _allChannelDisplays[5] = _totalChannelDisplay =
        new BandwidthChannelDisplay({
            NodeType::DomainServer, NodeType::EntityServer,
            NodeType::AudioMixer, NodeType::Agent,
            NodeType::AvatarMixer, NodeType::Unassigned
        }, form, "Total", "Kbps", 1.0, COLOR2);

    connect(averageUpdateTimer, SIGNAL(timeout()), this, SLOT(updateTimerTimeout()));
    averageUpdateTimer->start(1000);
}


BandwidthDialog::~BandwidthDialog() {
    for (unsigned int i = 0; i < _CHANNELCOUNT; i++) {
        delete _allChannelDisplays[i];
    }
}


void BandwidthDialog::updateTimerTimeout() {
    for (unsigned int i = 0; i < _CHANNELCOUNT; i++) {
        _allChannelDisplays[i]->bandwidthAverageUpdated();
    }
}


void BandwidthDialog::paintEvent(QPaintEvent* event) {
    for (unsigned int i=0; i<_CHANNELCOUNT; i++)
        _allChannelDisplays[i]->paint();
    this->QDialog::paintEvent(event);
}

void BandwidthDialog::reject() {

    // Just regularly close upon ESC
    this->QDialog::close();
}

void BandwidthDialog::closeEvent(QCloseEvent* event) {

    this->QDialog::closeEvent(event);
    emit closed();
}

