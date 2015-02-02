//
//  BandwidthDialog.h
//  interface/src/ui
//
//  Created by Tobias Schwinger on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BandwidthDialog_h
#define hifi_BandwidthDialog_h

#include <QDialog>
#include <QLabel>
#include <QFormLayout>

#include "BandwidthRecorder.h"


class BandwidthChannelDisplay : public QObject {
    Q_OBJECT

 public:
    BandwidthChannelDisplay(BandwidthRecorder::Channel *ch, QFormLayout* form);
    void Paint();

 private:
    BandwidthRecorder::Channel *ch;
    QLabel* label;
    QString strBuf;

 public slots:
     void bandwidthAverageUpdated();
};


class BandwidthDialog : public QDialog {
    Q_OBJECT
public:
    BandwidthDialog(QWidget* parent);
    ~BandwidthDialog();

    void paintEvent(QPaintEvent*);

private:
    BandwidthChannelDisplay* _audioChannelDisplay;
    BandwidthChannelDisplay* _avatarsChannelDisplay;
    BandwidthChannelDisplay* _octreeChannelDisplay;
    BandwidthChannelDisplay* _metavoxelsChannelDisplay;
    BandwidthChannelDisplay* _otherChannelDisplay;
    BandwidthChannelDisplay* _totalChannelDisplay; // sums of all the other channels

    static const unsigned int _CHANNELCOUNT = 6;
    BandwidthChannelDisplay *_allChannelDisplays[_CHANNELCOUNT];


signals:

    void closed();

public slots:

    void reject();
    void updateTimerTimeout();


protected:

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private:
    QTimer *averageUpdateTimer = new QTimer(this);

};

#endif // hifi_BandwidthDialog_h
