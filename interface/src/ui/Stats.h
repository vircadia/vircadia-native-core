//
//  Created by Bradley Austin Davis 2015/06/17
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stats_h
#define hifi_Stats_h

#include <OffscreenQmlElement.h>
#include <RenderArgs.h>
#include <QVector3D>
#include <AudioIOStats.h>

#define STATS_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue };


class Stats : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(bool timingExpanded READ isTimingExpanded NOTIFY timingExpandedChanged)
    Q_PROPERTY(QString monospaceFont READ monospaceFont CONSTANT)

    STATS_PROPERTY(int, serverCount, 0)
    // How often the app is creating new gpu::Frames
    STATS_PROPERTY(float, framerate, 0)
    // How often the display plugin is executing a given frame
    STATS_PROPERTY(float, renderrate, 0)
    // How often the display plugin is presenting to the device
    STATS_PROPERTY(float, presentrate, 0)
    
    STATS_PROPERTY(float, presentnewrate, 0)
    STATS_PROPERTY(float, presentdroprate, 0)
    STATS_PROPERTY(int, simrate, 0)
    STATS_PROPERTY(int, avatarSimrate, 0)
    STATS_PROPERTY(int, avatarCount, 0)
    STATS_PROPERTY(int, packetInCount, 0)
    STATS_PROPERTY(int, packetOutCount, 0)
    STATS_PROPERTY(float, mbpsIn, 0)
    STATS_PROPERTY(float, mbpsOut, 0)
    STATS_PROPERTY(float, assetMbpsIn, 0)
    STATS_PROPERTY(float, assetMbpsOut, 0)
    STATS_PROPERTY(int, audioPing, 0)
    STATS_PROPERTY(int, avatarPing, 0)
    STATS_PROPERTY(int, entitiesPing, 0)
    STATS_PROPERTY(int, assetPing, 0)
    STATS_PROPERTY(int, messagePing, 0)
    STATS_PROPERTY(QVector3D, position, QVector3D(0, 0, 0))
    STATS_PROPERTY(float, speed, 0)
    STATS_PROPERTY(float, yaw, 0)
    STATS_PROPERTY(int, avatarMixerInKbps, 0)
    STATS_PROPERTY(int, avatarMixerInPps, 0)
    STATS_PROPERTY(int, avatarMixerOutKbps, 0)
    STATS_PROPERTY(int, avatarMixerOutPps, 0)
    STATS_PROPERTY(int, audioMixerKbps, 0)
    STATS_PROPERTY(int, audioMixerPps, 0)
    STATS_PROPERTY(int, downloads, 0)
    STATS_PROPERTY(int, downloadLimit, 0)
    STATS_PROPERTY(int, downloadsPending, 0)
    Q_PROPERTY(QStringList downloadUrls READ downloadUrls NOTIFY downloadUrlsChanged)
    STATS_PROPERTY(int, triangles, 0)
    STATS_PROPERTY(int, quads, 0)
    STATS_PROPERTY(int, materialSwitches, 0)
    STATS_PROPERTY(int, itemConsidered, 0)
    STATS_PROPERTY(int, itemOutOfView, 0)
    STATS_PROPERTY(int, itemTooSmall, 0)
    STATS_PROPERTY(int, itemRendered, 0)
    STATS_PROPERTY(int, shadowConsidered, 0)
    STATS_PROPERTY(int, shadowOutOfView, 0)
    STATS_PROPERTY(int, shadowTooSmall, 0)
    STATS_PROPERTY(int, shadowRendered, 0)
    STATS_PROPERTY(QString, sendingMode, QString())
    STATS_PROPERTY(QString, packetStats, QString())
    STATS_PROPERTY(QString, lodStatus, QString())
    STATS_PROPERTY(QString, timingStats, QString())
    STATS_PROPERTY(int, serverElements, 0)
    STATS_PROPERTY(int, serverInternal, 0)
    STATS_PROPERTY(int, serverLeaves, 0)
    STATS_PROPERTY(int, localElements, 0)
    STATS_PROPERTY(int, localInternal, 0)
    STATS_PROPERTY(int, localLeaves, 0)
    STATS_PROPERTY(int, gpuBuffers, 0)
    STATS_PROPERTY(int, gpuTextures, 0)
    STATS_PROPERTY(int, gpuTexturesSparse, 0)
    STATS_PROPERTY(int, qmlTextureMemory, 0)
    STATS_PROPERTY(int, gpuTextureMemory, 0)
    STATS_PROPERTY(int, gpuTextureVirtualMemory, 0)
    STATS_PROPERTY(int, gpuTextureFramebufferMemory, 0)
    STATS_PROPERTY(int, gpuTextureSparseMemory, 0)
    STATS_PROPERTY(int, gpuSparseTextureEnabled, 0)
    STATS_PROPERTY(int, gpuFreeMemory, 0)

public:
    static Stats* getInstance();

    Stats(QQuickItem* parent = nullptr);
    bool includeTimingRecord(const QString& name);
    void setRenderDetails(const RenderDetails& details);
    const QString& monospaceFont() {
        return _monospaceFont;
    }

    void updateStats(bool force = false);

    bool isExpanded() { return _expanded; }
    bool isTimingExpanded() { return _timingExpanded; }

    void setExpanded(bool expanded) {
        if (_expanded != expanded) {
            _expanded = expanded;
            emit expandedChanged();
        }
    }

    QStringList downloadUrls () { return _downloadUrls; }

public slots:
    void forceUpdateStats() { updateStats(true); }

signals:
    void framerateChanged();
    void expandedChanged();
    void timingExpandedChanged();
    void serverCountChanged();
    void renderrateChanged();
    void presentrateChanged();
    void presentnewrateChanged();
    void presentdroprateChanged();
    void simrateChanged();
    void avatarSimrateChanged();
    void avatarCountChanged();
    void packetInCountChanged();
    void packetOutCountChanged();
    void mbpsInChanged();
    void mbpsOutChanged();
    void assetMbpsInChanged();
    void assetMbpsOutChanged();
    void audioPingChanged();
    void avatarPingChanged();
    void entitiesPingChanged();
    void assetPingChanged();
    void messagePingChanged();
    void positionChanged();
    void speedChanged();
    void yawChanged();
    void avatarMixerInKbpsChanged();
    void avatarMixerInPpsChanged();
    void avatarMixerOutKbpsChanged();
    void avatarMixerOutPpsChanged();
    void audioMixerKbpsChanged();
    void audioMixerPpsChanged();
    void downloadsChanged();
    void downloadLimitChanged();
    void downloadsPendingChanged();
    void downloadUrlsChanged();
    void trianglesChanged();
    void quadsChanged();
    void materialSwitchesChanged();
    void itemConsideredChanged();
    void itemOutOfViewChanged();
    void itemTooSmallChanged();
    void itemRenderedChanged();
    void shadowConsideredChanged();
    void shadowOutOfViewChanged();
    void shadowTooSmallChanged();
    void shadowRenderedChanged();
    void sendingModeChanged();
    void packetStatsChanged();
    void lodStatusChanged();
    void serverElementsChanged();
    void serverInternalChanged();
    void serverLeavesChanged();
    void localElementsChanged();
    void localInternalChanged();
    void localLeavesChanged();
    void timingStatsChanged();
    void qmlTextureMemoryChanged();
    void gpuBuffersChanged();
    void gpuTexturesChanged();
    void gpuTexturesSparseChanged();
    void gpuTextureMemoryChanged();
    void gpuTextureVirtualMemoryChanged();
    void gpuTextureFramebufferMemoryChanged();
    void gpuTextureSparseMemoryChanged();
    void gpuSparseTextureEnabledChanged();
    void gpuFreeMemoryChanged();

private:
    int _recentMaxPackets{ 0 } ; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon{ true };
    bool _expanded{ false };
    bool _timingExpanded{ false };
    QString _monospaceFont;
    const AudioIOStats* _audioStats;
    QStringList _downloadUrls = QStringList();
};

#endif // hifi_Stats_h

