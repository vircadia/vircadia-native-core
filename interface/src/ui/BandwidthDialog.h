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


class BandwidthDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI based on the configuration of the BandwidthRecorder
    BandwidthDialog(QWidget* parent, BandwidthRecorder* model);
    ~BandwidthDialog();

    class ChannelDisplay {
    public:
      ChannelDisplay(BandwidthRecorder::Channel *ch, QFormLayout* form);
      QLabel* setupLabel(QFormLayout* form);
      void setLabelText();

    private:
      BandwidthRecorder::Channel *ch;

      QLabel* label;
    };

    ChannelDisplay* audioChannelDisplay;
    ChannelDisplay* avatarsChannelDisplay;
    ChannelDisplay* octreeChannelDisplay;
    ChannelDisplay* metavoxelsChannelDisplay;

    // sums of all the other channels
    ChannelDisplay* totalChannelDisplay;

signals:

    void closed();

public slots:

    void reject();

protected:
    void paintEvent(QPaintEvent*);

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private:
    BandwidthRecorder* _model;
};

#endif // hifi_BandwidthDialog_h
