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

#include "BandwidthMeter.h"


class BandwidthDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI based on the configuration of the BandwidthMeter
    BandwidthDialog(QWidget* parent, BandwidthMeter* model);
    ~BandwidthDialog();

signals:

    void closed();

public slots:

    void reject();

protected:

    // State <- data model held by BandwidthMeter
    void paintEvent(QPaintEvent*);

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private:
    BandwidthMeter* _model;
    QLabel*         _labels[BandwidthMeter::N_STREAMS];
};

#endif // hifi_BandwidthDialog_h
