//
//  Stats.h
//  interface/src/ui
//
//  Created by Lucas Crisman on 22/03/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stats_h
#define hifi_Stats_h

#include <QObject>
#include <QQuickItem>
#include <QVector3D>

#include <OffscreenUi.h>
#include <RenderArgs.h>

#define STATS_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue }; 


class Stats : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL
    Q_PROPERTY(bool expanded READ isExpanded NOTIFY expandedChanged)
    STATS_PROPERTY(int, serverCount, 0)
    STATS_PROPERTY(int, framerate, 0)
    STATS_PROPERTY(int, avatarCount, 0)
    STATS_PROPERTY(int, packetInCount, 0)
    STATS_PROPERTY(int, packetOutCount, 0)
    STATS_PROPERTY(float, mbpsIn, 0)
    STATS_PROPERTY(float, mbpsOut, 0)
    STATS_PROPERTY(int, audioPing, 0)
    STATS_PROPERTY(int, avatarPing, 0)
    STATS_PROPERTY(int, entitiesPing, 0)
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
    STATS_PROPERTY(int, octreeElementsServer, 0)
    STATS_PROPERTY(int, octreeElementsLocal, 0)

public:
    static Stats* getInstance();

    Stats(QQuickItem* parent = nullptr);
    bool includeTimingRecord(const QString& name);
    void setRenderDetails(const RenderDetails& details);

    void updateStats();

    bool isExpanded() { return _expanded; }

    void setExpanded(bool expanded) {
        if (expanded != _expanded) {
            _expanded = expanded;
        }
    }

signals:
    void expandedChanged();
    void serverCountChanged();
    void framerateChanged();
    void avatarCountChanged();
    void packetInCountChanged();
    void packetOutCountChanged();
    void mbpsInChanged();
    void mbpsOutChanged();
    void audioPingChanged();
    void avatarPingChanged();
    void entitiesPingChanged();
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
    void octreeElementsServerChanged();
    void octreeElementsLocalChanged();

private:
    int _recentMaxPackets{ 0 } ; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon{ true };
    bool _expanded{ false };
};

#endif // hifi_Stats_h
