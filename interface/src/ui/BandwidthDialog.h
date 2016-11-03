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
#include <QVector>
#include <QTimer>

#include "Node.h"
#include "BandwidthRecorder.h"


const unsigned int COLOR0 = 0x33cc99ff;
const unsigned int COLOR1 = 0xffef40c0;
const unsigned int COLOR2 = 0xd0d0d0a0;


class BandwidthChannelDisplay : public QObject {
    Q_OBJECT

 public:
    BandwidthChannelDisplay(QVector<NodeType_t> nodeTypesToFollow,
                            QFormLayout* form,
                            char const* const caption, char const* unitCaption, float unitScale, unsigned colorRGBA);
    void paint();

 private:
    QVector<NodeType_t> _nodeTypesToFollow;
    QLabel* _label;
    QString _strBuf;
    char const* const _caption;
    char const* _unitCaption;
    float const _unitScale;
    unsigned _colorRGBA;


 public slots:
     void bandwidthAverageUpdated();
};


class BandwidthDialog : public QDialog {
    Q_OBJECT
public:
    BandwidthDialog(QWidget* parent);
    ~BandwidthDialog();

    void paintEvent(QPaintEvent*) override;

private:
    BandwidthChannelDisplay* _audioChannelDisplay;
    BandwidthChannelDisplay* _avatarsChannelDisplay;
    BandwidthChannelDisplay* _octreeChannelDisplay;
    BandwidthChannelDisplay* _domainChannelDisplay;
    BandwidthChannelDisplay* _otherChannelDisplay;
    BandwidthChannelDisplay* _totalChannelDisplay; // sums of all the other channels

    static const unsigned int _CHANNELCOUNT = 6;
    BandwidthChannelDisplay* _allChannelDisplays[_CHANNELCOUNT];


signals:

    void closed();

public slots:

    void reject() override;
    void updateTimerTimeout();


protected:

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*) override;

private:
    QTimer* averageUpdateTimer = new QTimer(this);

};

#endif // hifi_BandwidthDialog_h
