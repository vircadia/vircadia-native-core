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

#include "BandwidthMeter.h"


class BandwidthDialog : public QDialog {
    Q_OBJECT
public:

    // Sets up the UI based on the configuration of the BandwidthMeter
    BandwidthDialog(QWidget* parent, BandwidthMeter* model);

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

#endif /* defined(__interface__BandwidthDialog__) */

