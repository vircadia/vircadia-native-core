//
//  LodToolsDialog.h
//  interface/src/ui
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LodToolsDialog_h
#define hifi_LodToolsDialog_h

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QSlider;

class LodToolsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    LodToolsDialog(QWidget* parent);

signals:
    void closed();

public slots:
    void reject() override;
    void sizeScaleValueChanged(int value);
    void resetClicked(bool checked);
    void reloadSliders();
    void updateAutomaticLODAdjust();

protected:

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent* event) override;

private:
    QSlider* _lodSize;

    QCheckBox* _manualLODAdjust;

    QDoubleSpinBox* _desktopLODDecreaseFPS;

    QDoubleSpinBox* _hmdLODDecreaseFPS;

    QLabel* _feedback;
};

#endif // hifi_LodToolsDialog_h
