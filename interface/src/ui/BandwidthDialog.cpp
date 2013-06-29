
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
        snprintf(strBuf, sizeof(strBuf), "%010.5f%s", s.getValue() * ch.unitScale, ch.unitCaption);
        label->setText(strBuf);
    }

    this->QDialog::paintEvent(event);
    this->setFixedSize(this->width(), this->height());
}

void BandwidthDialog::closeEvent(QCloseEvent* event) {

    this->QDialog::closeEvent(event);
    emit closed();
}

