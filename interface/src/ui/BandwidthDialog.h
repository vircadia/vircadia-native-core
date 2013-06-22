//
//  BandwidthDialog.h
//  interface
//
//  Created by Tobias Schwinger on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__BandwidthDialog__
#define __hifi__BandwidthDialog__

#include <QDialog>

#include <QLabel>
#include <QDoubleSpinBox>

#include "BandwidthMeter.h"


class BandwidthDialog : public QDialog {
    Q_OBJECT
public:

    // Sets up the UI based on the configuration of the BandwidthMeter
    BandwidthDialog(QWidget* parent, BandwidthMeter* model);

signals:

    void closed();

protected:

    // State <- data model held by BandwidthMeter
    void paintEvent(QPaintEvent*);

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private slots:

    // State -> data model held by BandwidthMeter
    void applySettings(double);

private:
    BandwidthMeter* _model;
    QLabel*         _labels[BandwidthMeter::N_STREAMS];
    QDoubleSpinBox* _spinners[BandwidthMeter::N_CHANNELS];
};

#endif /* defined(__interface__BandwidthDialog__) */

