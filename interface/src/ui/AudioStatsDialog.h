//
//  AudioStatsDialog.h
//  hifi
//
//  Created by Bridget Went on 7/9/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef __hifi__AudioStatsDialog__
#define __hifi__AudioStatsDialog__

#include <stdio.h>

#include <QDialog>
#include <QLabel>
#include <QFormLayout>
#include <QVector>
#include <QTimer>
#include <QString>

#include <QObject>

#include <DependencyManager.h>

class AudioIOStats;
class AudioStreamStats;

//display
class AudioStatsDisplay : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    AudioStatsDisplay(QFormLayout* form, QString text, unsigned colorRGBA);
    void updatedDisplay(QString str);
    void paint();

private:
    QString _strBuf;
    QLabel* _label;
    QString _text;
    unsigned _colorRGBA;

};

//dialog
class AudioStatsDialog : public QDialog {
    Q_OBJECT
public:
    AudioStatsDialog(QWidget* parent);
    ~AudioStatsDialog();

    void paintEvent(QPaintEvent*) override;

private:
    // audio stats methods for rendering
    QVector<QString> _audioMixerStats;
    QVector<QString> _upstreamClientStats;
    QVector<QString> _upstreamMixerStats;
    QVector<QString> _downstreamStats;
    QVector<QString> _upstreamInjectedStats;

    int _audioMixerID;
    int _upstreamClientID;
    int _upstreamMixerID;
    int _downstreamID;
    int _upstreamInjectedID;

    QVector<QVector<AudioStatsDisplay*>> _audioDisplayChannels;

    void updateStats();
    int addChannel(QFormLayout* form, QVector<QString>& stats, const unsigned color);
    void updateChannel(QVector<QString>& stats, const int channelID);
    void updateChannels();
    void clearAllChannels();
    void renderAudioStreamStats(const AudioStreamStats* streamStats, QVector<QString>* audioStreamstats);


    const AudioIOStats* _stats;
    QFormLayout* _form;

    bool _shouldShowInjectedStreams{ false };


signals:


    void closed();

    public slots:


    void reject() override;
    void renderStats();

protected:

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*) override;

private:
    QTimer* averageUpdateTimer = new QTimer(this);
};





#endif /* defined(__hifi__AudioStatsDialog__) */

