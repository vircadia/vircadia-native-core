
#include "ui/BandwidthDialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>

#include "Log.h"

BandwidthDialog::BandwidthDialog(QWidget* parent, BandwidthMeter* model) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint),
    _model(model) {

    char strBuf[64];

    this->setWindowTitle("Bandwidth Details");

    // Create layouter
    QFormLayout* form = new QFormLayout();
    this->QDialog::setLayout(form);

    // Setup labels
    for (int i = 0; i < BandwidthMeter::N_STREAMS; ++i) {
        int chIdx = i / 2;
        bool input = i % 2 == 0;
        BandwidthMeter::ChannelInfo& ch = _model->channelInfo(chIdx);
        QLabel* label = _labels[i] = new QLabel();
        snprintf(strBuf, sizeof(strBuf), "%s %s Bandwidth:", input ? "Input" : "Output", ch.caption);
        form->addRow(strBuf, label);
    }

    // Setup spinners
    for (int i = 0; i < BandwidthMeter::N_CHANNELS; ++i) {

        BandwidthMeter::ChannelInfo& ch = _model->channelInfo(i);
        QDoubleSpinBox* spinner = _spinners[i] = new QDoubleSpinBox();
        spinner->setDecimals(3);
        spinner->setMinimum(0.001);
        spinner->setMaximum(1000.0);
        spinner->setSuffix(ch.unitCaption);
        snprintf(strBuf, sizeof(strBuf), "Maximum of %s Bandwidth Meter:", ch.caption);
        form->addRow(strBuf, spinner);
        connect(spinner, SIGNAL(valueChanged(double)), SLOT(applySettings(double)));
    }

}

void BandwidthDialog::paintEvent(QPaintEvent* event) {

    // Update labels
    char strBuf[64];
    for (int i = 0; i < BandwidthMeter::N_STREAMS; ++i) {
        int chIdx = i / 2;
        bool input = i % 2 == 0;
        BandwidthMeter::ChannelInfo& ch = _model->channelInfo(chIdx);
        BandwidthMeter::Stream& s = input ? _model->inputStream(chIdx) : _model->outputStream(chIdx);
        QLabel* label = _labels[i];
        snprintf(strBuf, sizeof(strBuf), "%010.6f%s", s.getValue() / ch.unitScale, ch.unitCaption);
        label->setText(strBuf);
    }
    // Update spinners (only when the value has been changed)
    for (int i = 0; i < BandwidthMeter::N_CHANNELS; ++i) {
        BandwidthMeter::ChannelInfo& ch = _model->channelInfo(i);
        if (_spinners[i]->value() != double(ch.unitsMax)) {
            _spinners[i]->setValue(ch.unitsMax);
        }
    }

    this->QDialog::paintEvent(event);
    this->setFixedSize(this->width(), this->height());
}

void BandwidthDialog::closeEvent(QCloseEvent* event) {

    this->QDialog::closeEvent(event);
    emit closed();
}

void BandwidthDialog::applySettings(double value) {

    // Update model from spinner value (only the one that's been changed)
    for (int i = 0; i < BandwidthMeter::N_CHANNELS; ++i) {
        float v = _spinners[i]->value();
        if (v == float(value)) {
            BandwidthMeter::ChannelInfo& ch = _model->channelInfo(i);
            if (ch.unitsMax != v) {
                ch.unitsMax = v;
            }
        }
    }
}

