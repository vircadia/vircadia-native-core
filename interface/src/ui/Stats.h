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
    STATS_PROPERTY(int, framerate, 0)
    STATS_PROPERTY(int, simrate, 0)
    STATS_PROPERTY(int, avatarCount, 0)
    STATS_PROPERTY(int, packetInCount, 0)
    STATS_PROPERTY(int, packetOutCount, 0)
    STATS_PROPERTY(float, mbpsIn, 0)
    STATS_PROPERTY(float, mbpsOut, 0)
    STATS_PROPERTY(int, audioPing, 0)
    STATS_PROPERTY(int, avatarPing, 0)
    STATS_PROPERTY(int, entitiesPing, 0)
    STATS_PROPERTY(int, assetPing, 0)
    STATS_PROPERTY(QVector3D, position, QVector3D(0, 0, 0) )
    STATS_PROPERTY(float, velocity, 0)
    STATS_PROPERTY(float, yaw, 0)
    STATS_PROPERTY(int, avatarMixerKbps, 0)
    STATS_PROPERTY(int, avatarMixerPps, 0)
    STATS_PROPERTY(int, audioMixerKbps, 0)
    STATS_PROPERTY(int, audioMixerPps, 0)
    STATS_PROPERTY(int, downloads, 0)
    STATS_PROPERTY(int, downloadsPending, 0)
    STATS_PROPERTY(int, triangles, 0)
    STATS_PROPERTY(int, quads, 0)
    STATS_PROPERTY(int, materialSwitches, 0)
    STATS_PROPERTY(int, meshOpaque, 0)
    STATS_PROPERTY(int, meshTranslucent, 0)
    STATS_PROPERTY(int, opaqueConsidered, 0)
    STATS_PROPERTY(int, opaqueOutOfView, 0)
    STATS_PROPERTY(int, opaqueTooSmall, 0)
    STATS_PROPERTY(int, translucentConsidered, 0)
    STATS_PROPERTY(int, translucentOutOfView, 0)
    STATS_PROPERTY(int, translucentTooSmall, 0)
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

public:
    static Stats* getInstance();

    Stats(QQuickItem* parent = nullptr);
    bool includeTimingRecord(const QString& name);
    void setRenderDetails(const RenderDetails& details);
    const QString& monospaceFont() {
        return _monospaceFont;
    }
    void updateStats();

    bool isExpanded() { return _expanded; }
    bool isTimingExpanded() { return _timingExpanded; }

    void setExpanded(bool expanded) {
        if (_expanded != expanded) {
            _expanded = expanded;
            emit expandedChanged();
        }
    }

signals:
    void expandedChanged();
    void timingExpandedChanged();
    void serverCountChanged();
    void framerateChanged();
    void simrateChanged();
    void avatarCountChanged();
    void packetInCountChanged();
    void packetOutCountChanged();
    void mbpsInChanged();
    void mbpsOutChanged();
    void audioPingChanged();
    void avatarPingChanged();
    void entitiesPingChanged();
    void assetPingChanged();
    void positionChanged();
    void velocityChanged();
    void yawChanged();
    void avatarMixerKbpsChanged();
    void avatarMixerPpsChanged();
    void audioMixerKbpsChanged();
    void audioMixerPpsChanged();
    void downloadsChanged();
    void downloadsPendingChanged();
    void trianglesChanged();
    void quadsChanged();
    void materialSwitchesChanged();
    void meshOpaqueChanged();
    void meshTranslucentChanged();
    void opaqueConsideredChanged();
    void opaqueOutOfViewChanged();
    void opaqueTooSmallChanged();
    void translucentConsideredChanged();
    void translucentOutOfViewChanged();
    void translucentTooSmallChanged();
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

private:
    int _recentMaxPackets{ 0 } ; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon{ true };
    bool _expanded{ false };
    bool _timingExpanded{ false };
    QString _monospaceFont;
};

#endif // hifi_Stats_h
