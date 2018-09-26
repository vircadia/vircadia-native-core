//
//  Created by Bradley Austin Davis 2015/06/17
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stats_h
#define hifi_Stats_h

#include <QtGui/QVector3D>

#include <OffscreenQmlElement.h>
#include <AudioIOStats.h>
#include <render/Args.h>

#define STATS_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue };

/**jsdoc
 * @namespace Stats
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {boolean} expanded
 * @property {boolean} timingExpanded - <em>Read-only.</em>
 * @property {string} monospaceFont - <em>Read-only.</em>
 *
 * @property {number} serverCount - <em>Read-only.</em>
 * @property {number} renderrate - How often the app is creating new gpu::Frames. <em>Read-only.</em>
 * @property {number} presentrate - How often the display plugin is presenting to the device. <em>Read-only.</em>
 * @property {number} stutterrate - How often the display device is reprojecting old frames. <em>Read-only.</em>
 *
 * @property {number} appdropped - <em>Read-only.</em>
 * @property {number} longsubmits - <em>Read-only.</em>
 * @property {number} longrenders - <em>Read-only.</em>
 * @property {number} longframes - <em>Read-only.</em>
 *
 * @property {number} presentnewrate - <em>Read-only.</em>
 * @property {number} presentdroprate - <em>Read-only.</em>
 * @property {number} gameLoopRate - <em>Read-only.</em>
 * @property {number} avatarCount - <em>Read-only.</em>
 * @property {number} physicsObjectCount - <em>Read-only.</em>
 * @property {number} updatedAvatarCount - <em>Read-only.</em>
 * @property {number} notUpdatedAvatarCount - <em>Read-only.</em>
 * @property {number} packetInCount - <em>Read-only.</em>
 * @property {number} packetOutCount - <em>Read-only.</em>
 * @property {number} mbpsIn - <em>Read-only.</em>
 * @property {number} mbpsOut - <em>Read-only.</em>
 * @property {number} assetMbpsIn - <em>Read-only.</em>
 * @property {number} assetMbpsOut - <em>Read-only.</em>
 * @property {number} audioPing - <em>Read-only.</em>
 * @property {number} avatarPing - <em>Read-only.</em>
 * @property {number} entitiesPing - <em>Read-only.</em>
 * @property {number} assetPing - <em>Read-only.</em>
 * @property {number} messagePing - <em>Read-only.</em>
 * @property {Vec3} position - <em>Read-only.</em>
 * @property {number} speed - <em>Read-only.</em>
 * @property {number} yaw - <em>Read-only.</em>
 * @property {number} avatarMixerInKbps - <em>Read-only.</em>
 * @property {number} avatarMixerInPps - <em>Read-only.</em>
 * @property {number} avatarMixerOutKbps - <em>Read-only.</em>
 * @property {number} avatarMixerOutPps - <em>Read-only.</em>
 * @property {number} myAvatarSendRate - <em>Read-only.</em>
 *
 * @property {number} audioMixerInKbps - <em>Read-only.</em>
 * @property {number} audioMixerInPps - <em>Read-only.</em>
 * @property {number} audioMixerOutKbps - <em>Read-only.</em>
 * @property {number} audioMixerOutPps - <em>Read-only.</em>
 * @property {number} audioMixerKbps - <em>Read-only.</em>
 * @property {number} audioMixerPps - <em>Read-only.</em>
 * @property {number} audioOutboundPPS - <em>Read-only.</em>
 * @property {number} audioSilentOutboundPPS - <em>Read-only.</em>
 * @property {number} audioAudioInboundPPS - <em>Read-only.</em>
 * @property {number} audioSilentInboundPPS - <em>Read-only.</em>
 * @property {number} audioPacketLoss - <em>Read-only.</em>
 * @property {string} audioCodec - <em>Read-only.</em>
 * @property {string} audioNoiseGate - <em>Read-only.</em>
 * @property {number} entityPacketsInKbps - <em>Read-only.</em>
 *
 * @property {number} downloads - <em>Read-only.</em>
 * @property {number} downloadLimit - <em>Read-only.</em>
 * @property {number} downloadsPending - <em>Read-only.</em>
 * @property {string[]} downloadUrls - <em>Read-only.</em>
 * @property {number} processing - <em>Read-only.</em>
 * @property {number} processingPending - <em>Read-only.</em>
 * @property {number} triangles - <em>Read-only.</em>
 * @property {number} materialSwitches - <em>Read-only.</em>
 * @property {number} itemConsidered - <em>Read-only.</em>
 * @property {number} itemOutOfView - <em>Read-only.</em>
 * @property {number} itemTooSmall - <em>Read-only.</em>
 * @property {number} itemRendered - <em>Read-only.</em>
 * @property {number} shadowConsidered - <em>Read-only.</em>
 * @property {number} shadowOutOfView - <em>Read-only.</em>
 * @property {number} shadowTooSmall - <em>Read-only.</em>
 * @property {number} shadowRendered - <em>Read-only.</em>
 * @property {string} sendingMode - <em>Read-only.</em>
 * @property {string} packetStats - <em>Read-only.</em>
 * @property {string} lodStatus - <em>Read-only.</em>
 * @property {string} timingStats - <em>Read-only.</em>
 * @property {string} gameUpdateStats - <em>Read-only.</em>
 * @property {number} serverElements - <em>Read-only.</em>
 * @property {number} serverInternal - <em>Read-only.</em>
 * @property {number} serverLeaves - <em>Read-only.</em>
 * @property {number} localElements - <em>Read-only.</em>
 * @property {number} localInternal - <em>Read-only.</em>
 * @property {number} localLeaves - <em>Read-only.</em>
 * @property {number} rectifiedTextureCount - <em>Read-only.</em>
 * @property {number} decimatedTextureCount - <em>Read-only.</em>
 * @property {number} gpuBuffers - <em>Read-only.</em>
 * @property {number} gpuBufferMemory - <em>Read-only.</em>
 * @property {number} gpuTextures - <em>Read-only.</em>
 * @property {number} glContextSwapchainMemory - <em>Read-only.</em>
 * @property {number} qmlTextureMemory - <em>Read-only.</em>
 * @property {number} texturePendingTransfers - <em>Read-only.</em>
 * @property {number} gpuTextureMemory - <em>Read-only.</em>
 * @property {number} gpuTextureResidentMemory - <em>Read-only.</em>
 * @property {number} gpuTextureFramebufferMemory - <em>Read-only.</em>
 * @property {number} gpuTextureResourceMemory - <em>Read-only.</em>
 * @property {number} gpuTextureResourceIdealMemory - <em>Read-only.</em>
 * @property {number} gpuTextureResourcePopulatedMemory - <em>Read-only.</em>
 * @property {number} gpuTextureExternalMemory - <em>Read-only.</em>
 * @property {string} gpuTextureMemoryPressureState - <em>Read-only.</em>
 * @property {number} gpuFreeMemory - <em>Read-only.</em>
 * @property {number} gpuFrameTime - <em>Read-only.</em>
 * @property {number} batchFrameTime - <em>Read-only.</em>
 * @property {number} engineFrameTime - <em>Read-only.</em>
 * @property {number} avatarSimulationTime - <em>Read-only.</em>
 *
 *
 * @property {number} x
 * @property {number} y
 * @property {number} z
 * @property {number} width
 * @property {number} height
 *
 * @property {number} opacity
 * @property {boolean} enabled
 * @property {boolean} visible
 *
 * @property {string} state
 * @property {object} anchors - <em>Read-only.</em>
 * @property {number} baselineOffset
 *
 * @property {boolean} clip
 *
 * @property {boolean} focus
 * @property {boolean} activeFocus - <em>Read-only.</em>
 * @property {boolean} activeFocusOnTab
 *
 * @property {number} rotation
 * @property {number} scale
 * @property {number} transformOrigin
 *
 * @property {boolean} smooth
 * @property {boolean} antialiasing
 * @property {number} implicitWidth
 * @property {number} implicitHeight
 *
 * @property {object} layer - <em>Read-only.</em>

 * @property {number} stylusPicksCount - <em>Read-only.</em>
 * @property {number} rayPicksCount - <em>Read-only.</em>
 * @property {number} parabolaPicksCount - <em>Read-only.</em>
 * @property {number} collisionPicksCount - <em>Read-only.</em>
 * @property {Vec4} stylusPicksUpdated - <em>Read-only.</em>
 * @property {Vec4} rayPicksUpdated - <em>Read-only.</em>
 * @property {Vec4} parabolaPicksUpdated - <em>Read-only.</em>
 * @property {Vec4} collisionPicksUpdated - <em>Read-only.</em>
 */
// Properties from x onwards are QQuickItem properties.

class Stats : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(bool timingExpanded READ isTimingExpanded NOTIFY timingExpandedChanged)
    Q_PROPERTY(QString monospaceFont READ monospaceFont CONSTANT)

    STATS_PROPERTY(int, serverCount, 0)
    // How often the app is creating new gpu::Frames
    STATS_PROPERTY(float, renderrate, 0)
    // How often the display plugin is presenting to the device
    STATS_PROPERTY(float, presentrate, 0)
    // How often the display device reprojecting old frames
    STATS_PROPERTY(float, stutterrate, 0)

    STATS_PROPERTY(int, appdropped, 0)
    STATS_PROPERTY(int, longsubmits, 0)
    STATS_PROPERTY(int, longrenders, 0)
    STATS_PROPERTY(int, longframes, 0)

    STATS_PROPERTY(float, presentnewrate, 0)
    STATS_PROPERTY(float, presentdroprate, 0)
    STATS_PROPERTY(int, gameLoopRate, 0)
    STATS_PROPERTY(int, avatarCount, 0)
    STATS_PROPERTY(int, physicsObjectCount, 0)
    STATS_PROPERTY(int, updatedAvatarCount, 0)
    STATS_PROPERTY(int, notUpdatedAvatarCount, 0)
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
    STATS_PROPERTY(float, myAvatarSendRate, 0)

    STATS_PROPERTY(int, audioMixerInKbps, 0)
    STATS_PROPERTY(int, audioMixerInPps, 0)
    STATS_PROPERTY(int, audioMixerOutKbps, 0)
    STATS_PROPERTY(int, audioMixerOutPps, 0)
    STATS_PROPERTY(int, audioMixerKbps, 0)
    STATS_PROPERTY(int, audioMixerPps, 0)
    STATS_PROPERTY(int, audioOutboundPPS, 0)
    STATS_PROPERTY(int, audioSilentOutboundPPS, 0)
    STATS_PROPERTY(int, audioAudioInboundPPS, 0)
    STATS_PROPERTY(int, audioSilentInboundPPS, 0)
    STATS_PROPERTY(int, audioPacketLoss, 0)
    STATS_PROPERTY(QString, audioCodec, QString())
    STATS_PROPERTY(QString, audioNoiseGate, QString())
    STATS_PROPERTY(int, entityPacketsInKbps, 0)

    STATS_PROPERTY(int, downloads, 0)
    STATS_PROPERTY(int, downloadLimit, 0)
    STATS_PROPERTY(int, downloadsPending, 0)
    Q_PROPERTY(QStringList downloadUrls READ downloadUrls NOTIFY downloadUrlsChanged)
    STATS_PROPERTY(int, processing, 0)
    STATS_PROPERTY(int, processingPending, 0)
    STATS_PROPERTY(int, triangles, 0)
    STATS_PROPERTY(int, drawcalls, 0)
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
    STATS_PROPERTY(QString, gameUpdateStats, QString())
    STATS_PROPERTY(int, serverElements, 0)
    STATS_PROPERTY(int, serverInternal, 0)
    STATS_PROPERTY(int, serverLeaves, 0)
    STATS_PROPERTY(int, localElements, 0)
    STATS_PROPERTY(int, localInternal, 0)
    STATS_PROPERTY(int, localLeaves, 0)
    STATS_PROPERTY(int, rectifiedTextureCount, 0)
    STATS_PROPERTY(int, decimatedTextureCount, 0)
    STATS_PROPERTY(int, gpuBuffers, 0)
    STATS_PROPERTY(int, gpuBufferMemory, 0)
    STATS_PROPERTY(int, gpuTextures, 0)
    STATS_PROPERTY(int, glContextSwapchainMemory, 0)
    STATS_PROPERTY(int, qmlTextureMemory, 0)
    STATS_PROPERTY(int, texturePendingTransfers, 0)
    STATS_PROPERTY(int, gpuTextureMemory, 0)
    STATS_PROPERTY(int, gpuTextureResidentMemory, 0)
    STATS_PROPERTY(int, gpuTextureFramebufferMemory, 0)
    STATS_PROPERTY(int, gpuTextureResourceMemory, 0)
    STATS_PROPERTY(int, gpuTextureResourceIdealMemory, 0)
    STATS_PROPERTY(int, gpuTextureResourcePopulatedMemory, 0)
    STATS_PROPERTY(int, gpuTextureExternalMemory, 0)
    STATS_PROPERTY(QString, gpuTextureMemoryPressureState, QString())
    STATS_PROPERTY(int, gpuFreeMemory, 0)
    STATS_PROPERTY(QVector2D, gpuFrameSize, QVector2D(0,0))
    STATS_PROPERTY(float, gpuFrameTime, 0)
    STATS_PROPERTY(float, gpuFrameTimePerPixel, 0)
    STATS_PROPERTY(float, batchFrameTime, 0)
    STATS_PROPERTY(float, engineFrameTime, 0)
    STATS_PROPERTY(float, avatarSimulationTime, 0)

    STATS_PROPERTY(int, stylusPicksCount, 0)
    STATS_PROPERTY(int, rayPicksCount, 0)
    STATS_PROPERTY(int, parabolaPicksCount, 0)
    STATS_PROPERTY(int, collisionPicksCount, 0)
    STATS_PROPERTY(QVector4D, stylusPicksUpdated, QVector4D(0, 0, 0, 0))
    STATS_PROPERTY(QVector4D, rayPicksUpdated, QVector4D(0, 0, 0, 0))
    STATS_PROPERTY(QVector4D, parabolaPicksUpdated, QVector4D(0, 0, 0, 0))
    STATS_PROPERTY(QVector4D, collisionPicksUpdated, QVector4D(0, 0, 0, 0))

public:
    static Stats* getInstance();

    Stats(QQuickItem* parent = nullptr);
    bool includeTimingRecord(const QString& name);
    void setRenderDetails(const render::RenderDetails& details);
    const QString& monospaceFont() {
        return _monospaceFont;
    }

    void updateStats(bool force = false);

    bool isExpanded() { return _expanded; }
    bool isTimingExpanded() { return _showTimingDetails; }

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

    /**jsdoc
     * Triggered when the value of the <code>longsubmits</code> property changes.
     * @function Stats.longsubmitsChanged
     * @returns {Signal}
     */
    void longsubmitsChanged();

    /**jsdoc
     * Triggered when the value of the <code>longrenders</code> property changes.
     * @function Stats.longrendersChanged
     * @returns {Signal}
     */
    void longrendersChanged();

    /**jsdoc
     * Triggered when the value of the <code>longframes</code> property changes.
     * @function Stats.longframesChanged
     * @returns {Signal}
     */
    void longframesChanged();

    /**jsdoc
     * Triggered when the value of the <code>appdropped</code> property changes.
     * @function Stats.appdroppedChanged
     * @returns {Signal}
     */
    void appdroppedChanged();

    /**jsdoc
     * Triggered when the value of the <code>expanded</code> property changes.
     * @function Stats.expandedChanged
     * @returns {Signal}
     */
    void expandedChanged();

    /**jsdoc
     * Triggered when the value of the <code>timingExpanded</code> property changes.
     * @function Stats.timingExpandedChanged
     * @returns {Signal}
     */
    void timingExpandedChanged();

    /**jsdoc
     * Triggered when the value of the <code>serverCount</code> property changes.
     * @function Stats.serverCountChanged
     * @returns {Signal}
     */
    void serverCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>renderrate</code> property changes.
     * @function Stats.renderrateChanged
     * @returns {Signal}
     */
    void renderrateChanged();

    /**jsdoc
     * Triggered when the value of the <code>presentrate</code> property changes.
     * @function Stats.presentrateChanged
     * @returns {Signal}
     */
    void presentrateChanged();

    /**jsdoc
     * Triggered when the value of the <code>presentnewrate</code> property changes.
     * @function Stats.presentnewrateChanged
     * @returns {Signal}
     */
    void presentnewrateChanged();

    /**jsdoc
     * Triggered when the value of the <code>presentdroprate</code> property changes.
     * @function Stats.presentdroprateChanged
     * @returns {Signal}
     */
    void presentdroprateChanged();

    /**jsdoc
     * Triggered when the value of the <code>stutterrate</code> property changes.
     * @function Stats.stutterrateChanged
     * @returns {Signal}
     */
    void stutterrateChanged();

    /**jsdoc
     * Triggered when the value of the <code>gameLoopRate</code> property changes.
     * @function Stats.gameLoopRateChanged
     * @returns {Signal}
     */
    void gameLoopRateChanged();

    /**jsdoc
     * Trigered when
     * @function Stats.numPhysicsBodiesChanged
     * @returns {Signal}
     */
    void physicsObjectCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarCount</code> property changes.
     * @function Stats.avatarCountChanged
     * @returns {Signal}
     */
    void avatarCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>updatedAvatarCount</code> property changes.
     * @function Stats.updatedAvatarCountChanged
     * @returns {Signal}
     */
    void updatedAvatarCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>notUpdatedAvatarCount</code> property changes.
     * @function Stats.notUpdatedAvatarCountChanged
     * @returns {Signal}
     */
    void notUpdatedAvatarCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>packetInCount</code> property changes.
     * @function Stats.packetInCountChanged
     * @returns {Signal}
     */
    void packetInCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>packetOutCount</code> property changes.
     * @function Stats.packetOutCountChanged
     * @returns {Signal}
     */
    void packetOutCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>mbpsIn</code> property changes.
     * @function Stats.mbpsInChanged
     * @returns {Signal}
     */
    void mbpsInChanged();

    /**jsdoc
     * Triggered when the value of the <code>mbpsOut</code> property changes.
     * @function Stats.mbpsOutChanged
     * @returns {Signal}
     */
    void mbpsOutChanged();

    /**jsdoc
     * Triggered when the value of the <code>assetMbpsIn</code> property changes.
     * @function Stats.assetMbpsInChanged
     * @returns {Signal}
     */
    void assetMbpsInChanged();

    /**jsdoc
     * Triggered when the value of the <code>assetMbpsOut</code> property changes.
     * @function Stats.assetMbpsOutChanged
     * @returns {Signal}
     */
    void assetMbpsOutChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioPing</code> property changes.
     * @function Stats.audioPingChanged
     * @returns {Signal}
     */
    void audioPingChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarPing</code> property changes.
     * @function Stats.avatarPingChanged
     * @returns {Signal}
     */
    void avatarPingChanged();

    /**jsdoc
     * Triggered when the value of the <code>entitiesPing</code> property changes.
     * @function Stats.entitiesPingChanged
     * @returns {Signal}
     */
    void entitiesPingChanged();

    /**jsdoc
     * Triggered when the value of the <code>assetPing</code> property changes.
     * @function Stats.assetPingChanged
     * @returns {Signal}
     */
    void assetPingChanged();

    /**jsdoc
     * Triggered when the value of the <code>messagePing</code> property changes.
     * @function Stats.messagePingChanged
     * @returns {Signal}
     */
    void messagePingChanged();

    /**jsdoc
     * Triggered when the value of the <code>position</code> property changes.
     * @function Stats.positionChanged
     * @returns {Signal}
     */
    void positionChanged();

    /**jsdoc
     * Triggered when the value of the <code>speed</code> property changes.
     * @function Stats.speedChanged
     * @returns {Signal}
     */
    void speedChanged();

    /**jsdoc
     * Triggered when the value of the <code>yaw</code> property changes.
     * @function Stats.yawChanged
     * @returns {Signal}
     */
    void yawChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarMixerInKbps</code> property changes.
     * @function Stats.avatarMixerInKbpsChanged
     * @returns {Signal}
     */
    void avatarMixerInKbpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarMixerInPps</code> property changes.
     * @function Stats.avatarMixerInPpsChanged
     * @returns {Signal}
     */
    void avatarMixerInPpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarMixerOutKbps</code> property changes.
     * @function Stats.avatarMixerOutKbpsChanged
     * @returns {Signal}
     */
    void avatarMixerOutKbpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarMixerOutPps</code> property changes.
     * @function Stats.avatarMixerOutPpsChanged
     * @returns {Signal}
     */
    void avatarMixerOutPpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>myAvatarSendRate</code> property changes.
     * @function Stats.myAvatarSendRateChanged
     * @returns {Signal}
     */
    void myAvatarSendRateChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioMixerInKbps</code> property changes.
     * @function Stats.audioMixerInKbpsChanged
     * @returns {Signal}
     */
    void audioMixerInKbpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioMixerInPps</code> property changes.
     * @function Stats.audioMixerInPpsChanged
     * @returns {Signal}
     */
    void audioMixerInPpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioMixerOutKbps</code> property changes.
     * @function Stats.audioMixerOutKbpsChanged
     * @returns {Signal}
     */
    void audioMixerOutKbpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioMixerOutPps</code> property changes.
     * @function Stats.audioMixerOutPpsChanged
     * @returns {Signal}
     */
    void audioMixerOutPpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioMixerKbps</code> property changes.
     * @function Stats.audioMixerKbpsChanged
     * @returns {Signal}
     */
    void audioMixerKbpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioMixerPps</code> property changes.
     * @function Stats.audioMixerPpsChanged
     * @returns {Signal}
     */
    void audioMixerPpsChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioOutboundPPS</code> property changes.
     * @function Stats.audioOutboundPPSChanged
     * @returns {Signal}
     */
    void audioOutboundPPSChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioSilentOutboundPPS</code> property changes.
     * @function Stats.audioSilentOutboundPPSChanged
     * @returns {Signal}
     */
    void audioSilentOutboundPPSChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioAudioInboundPPS</code> property changes.
     * @function Stats.audioAudioInboundPPSChanged
     * @returns {Signal}
     */
    void audioAudioInboundPPSChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioSilentInboundPPS</code> property changes.
     * @function Stats.audioSilentInboundPPSChanged
     * @returns {Signal}
     */
    void audioSilentInboundPPSChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioPacketLoss</code> property changes.
     * @function Stats.audioPacketLossChanged
     * @returns {Signal}
     */
    void audioPacketLossChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioCodec</code> property changes.
     * @function Stats.audioCodecChanged
     * @returns {Signal}
     */
    void audioCodecChanged();

    /**jsdoc
     * Triggered when the value of the <code>audioNoiseGate</code> property changes.
     * @function Stats.audioNoiseGateChanged
     * @returns {Signal}
     */
    void audioNoiseGateChanged();

    /**jsdoc
     * Triggered when the value of the <code>entityPacketsInKbps</code> property changes.
     * @function Stats.entityPacketsInKbpsChanged
     * @returns {Signal}
     */
    void entityPacketsInKbpsChanged();


    /**jsdoc
     * Triggered when the value of the <code>downloads</code> property changes.
     * @function Stats.downloadsChanged
     * @returns {Signal}
     */
    void downloadsChanged();

    /**jsdoc
     * Triggered when the value of the <code>downloadLimit</code> property changes.
     * @function Stats.downloadLimitChanged
     * @returns {Signal}
     */
    void downloadLimitChanged();

    /**jsdoc
     * Triggered when the value of the <code>downloadsPending</code> property changes.
     * @function Stats.downloadsPendingChanged
     * @returns {Signal}
     */
    void downloadsPendingChanged();

    /**jsdoc
     * Triggered when the value of the <code>downloadUrls</code> property changes.
     * @function Stats.downloadUrlsChanged
     * @returns {Signal}
     */
    void downloadUrlsChanged();

    /**jsdoc
     * Triggered when the value of the <code>processing</code> property changes.
     * @function Stats.processingChanged
     * @returns {Signal}
     */
    void processingChanged();

    /**jsdoc
     * Triggered when the value of the <code>processingPending</code> property changes.
     * @function Stats.processingPendingChanged
     * @returns {Signal}
     */
    void processingPendingChanged();

    /**jsdoc
     * Triggered when the value of the <code>triangles</code> property changes.
     * @function Stats.trianglesChanged
     * @returns {Signal}
     */
    void trianglesChanged();

    /**jsdoc
    * Triggered when the value of the <code>drawcalls</code> property changes.
    * This 
    * @function Stats.drawcallsChanged
    * @returns {Signal}
    */
    void drawcallsChanged();

    /**jsdoc
     * Triggered when the value of the <code>materialSwitches</code> property changes.
     * @function Stats.materialSwitchesChanged
     * @returns {Signal}
     */
    void materialSwitchesChanged();

    /**jsdoc
     * Triggered when the value of the <code>itemConsidered</code> property changes.
     * @function Stats.itemConsideredChanged
     * @returns {Signal}
     */
    void itemConsideredChanged();

    /**jsdoc
     * Triggered when the value of the <code>itemOutOfView</code> property changes.
     * @function Stats.itemOutOfViewChanged
     * @returns {Signal}
     */
    void itemOutOfViewChanged();

    /**jsdoc
     * Triggered when the value of the <code>itemTooSmall</code> property changes.
     * @function Stats.itemTooSmallChanged
     * @returns {Signal}
     */
    void itemTooSmallChanged();

    /**jsdoc
     * Triggered when the value of the <code>itemRendered</code> property changes.
     * @function Stats.itemRenderedChanged
     * @returns {Signal}
     */
    void itemRenderedChanged();

    /**jsdoc
     * Triggered when the value of the <code>shadowConsidered</code> property changes.
     * @function Stats.shadowConsideredChanged
     * @returns {Signal}
     */
    void shadowConsideredChanged();

    /**jsdoc
     * Triggered when the value of the <code>shadowOutOfView</code> property changes.
     * @function Stats.shadowOutOfViewChanged
     * @returns {Signal}
     */
    void shadowOutOfViewChanged();

    /**jsdoc
     * Triggered when the value of the <code>shadowTooSmall</code> property changes.
     * @function Stats.shadowTooSmallChanged
     * @returns {Signal}
     */
    void shadowTooSmallChanged();

    /**jsdoc
     * Triggered when the value of the <code>shadowRendered</code> property changes.
     * @function Stats.shadowRenderedChanged
     * @returns {Signal}
     */
    void shadowRenderedChanged();

    /**jsdoc
     * Triggered when the value of the <code>sendingMode</code> property changes.
     * @function Stats.sendingModeChanged
     * @returns {Signal}
     */
    void sendingModeChanged();

    /**jsdoc
     * Triggered when the value of the <code>packetStats</code> property changes.
     * @function Stats.packetStatsChanged
     * @returns {Signal}
     */
    void packetStatsChanged();

    /**jsdoc
     * Triggered when the value of the <code>lodStatus</code> property changes.
     * @function Stats.lodStatusChanged
     * @returns {Signal}
     */
    void lodStatusChanged();

    /**jsdoc
     * Triggered when the value of the <code>serverElements</code> property changes.
     * @function Stats.serverElementsChanged
     * @returns {Signal}
     */
    void serverElementsChanged();

    /**jsdoc
     * Triggered when the value of the <code>serverInternal</code> property changes.
     * @function Stats.serverInternalChanged
     * @returns {Signal}
     */
    void serverInternalChanged();

    /**jsdoc
     * Triggered when the value of the <code>serverLeaves</code> property changes.
     * @function Stats.serverLeavesChanged
     * @returns {Signal}
     */
    void serverLeavesChanged();

    /**jsdoc
     * Triggered when the value of the <code>localElements</code> property changes.
     * @function Stats.localElementsChanged
     * @returns {Signal}
     */
    void localElementsChanged();

    /**jsdoc
     * Triggered when the value of the <code>localInternal</code> property changes.
     * @function Stats.localInternalChanged
     * @returns {Signal}
     */
    void localInternalChanged();

    /**jsdoc
     * Triggered when the value of the <code>localLeaves</code> property changes.
     * @function Stats.localLeavesChanged
     * @returns {Signal}
     */
    void localLeavesChanged();

    /**jsdoc
     * Triggered when the value of the <code>timingStats</code> property changes.
     * @function Stats.timingStatsChanged
     * @returns {Signal}
     */
    void timingStatsChanged();

    /**jsdoc
     * Triggered when the value of the <code>gameUpdateStats</code> property changes.
     * @function Stats.gameUpdateStatsChanged
     * @returns {Signal}
     */
    void gameUpdateStatsChanged();

    /**jsdoc
     * Triggered when the value of the <code>glContextSwapchainMemory</code> property changes.
     * @function Stats.glContextSwapchainMemoryChanged
     * @returns {Signal}
     */
    void glContextSwapchainMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>qmlTextureMemory</code> property changes.
     * @function Stats.qmlTextureMemoryChanged
     * @returns {Signal}
     */
    void qmlTextureMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>texturePendingTransfers</code> property changes.
     * @function Stats.texturePendingTransfersChanged
     * @returns {Signal}
     */
    void texturePendingTransfersChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuBuffers</code> property changes.
     * @function Stats.gpuBuffersChanged
     * @returns {Signal}
     */
    void gpuBuffersChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuBufferMemory</code> property changes.
     * @function Stats.gpuBufferMemoryChanged
     * @returns {Signal}
     */
    void gpuBufferMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextures</code> property changes.
     * @function Stats.gpuTexturesChanged
     * @returns {Signal}
     */
    void gpuTexturesChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureMemory</code> property changes.
     * @function Stats.gpuTextureMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureResidentMemory</code> property changes.
     * @function Stats.gpuTextureResidentMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResidentMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureFramebufferMemory</code> property changes.
     * @function Stats.gpuTextureFramebufferMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureFramebufferMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureResourceMemory</code> property changes.
     * @function Stats.gpuTextureResourceMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResourceMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureResourceIdealMemory</code> property changes.
     * @function Stats.gpuTextureResourceIdealMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResourceIdealMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureResourcePopulatedMemory</code> property changes.
     * @function Stats.gpuTextureResourcePopulatedMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResourcePopulatedMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureExternalMemory</code> property changes.
     * @function Stats.gpuTextureExternalMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureExternalMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuTextureMemoryPressureState</code> property changes.
     * @function Stats.gpuTextureMemoryPressureStateChanged
     * @returns {Signal}
     */
    void gpuTextureMemoryPressureStateChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuFreeMemory</code> property changes.
     * @function Stats.gpuFreeMemoryChanged
     * @returns {Signal}
     */
    void gpuFreeMemoryChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuFrameTime</code> property changes.
     * @function Stats.gpuFrameTimeChanged
     * @returns {Signal}
     */
    void gpuFrameTimeChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuFrameTime</code> property changes.
     * @function Stats.gpuFrameTimeChanged
     * @returns {Signal}
     */
    void gpuFrameSizeChanged();

    /**jsdoc
     * Triggered when the value of the <code>gpuFrameTime</code> property changes.
     * @function Stats.gpuFrameTimeChanged
     * @returns {Signal}
     */
    void gpuFrameTimePerPixelChanged();

    /**jsdoc
     * Triggered when the value of the <code>batchFrameTime</code> property changes.
     * @function Stats.batchFrameTimeChanged
     * @returns {Signal}
     */
    void batchFrameTimeChanged();

    /**jsdoc
     * Triggered when the value of the <code>engineFrameTime</code> property changes.
     * @function Stats.engineFrameTimeChanged
     * @returns {Signal}
     */
    void engineFrameTimeChanged();

    /**jsdoc
     * Triggered when the value of the <code>avatarSimulationTime</code> property changes.
     * @function Stats.avatarSimulationTimeChanged
     * @returns {Signal}
     */
    void avatarSimulationTimeChanged();

    /**jsdoc
     * Triggered when the value of the <code>rectifiedTextureCount</code> property changes.
     * @function Stats.rectifiedTextureCountChanged
     * @returns {Signal}
     */
    void rectifiedTextureCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>decimatedTextureCount</code> property changes.
     * @function Stats.decimatedTextureCountChanged
     * @returns {Signal}
     */
    void decimatedTextureCountChanged();

    // QQuickItem signals.

    /**jsdoc
     * Triggered when the parent item changes.
     * @function Stats.parentChanged
     * @param {object} parent
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>x</code> property changes.
     * @function Stats.xChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>y</code> property changes.
     * @function Stats.yChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>z</code> property changes.
     * @function Stats.zChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>width</code> property changes.
     * @function Stats.widthChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>height</code> property changes.
     * @function Stats.heightChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>opacity</code> property changes.
     * @function Stats.opacityChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>enabled</code> property changes.
     * @function Stats.enabledChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>visibleChanged</code> property changes.
     * @function Stats.visibleChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the list of visible children changes.
     * @function Stats.visibleChildrenChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>state</code> property changes.
     * @function Stats.stateChanged
     * @paramm {string} state
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the position and size of the rectangle containing the children changes.
     * @function Stats.childrenRectChanged
     * @param {Rect} childrenRect
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>baselineOffset</code> property changes.
     * @function Stats.baselineOffsetChanged
     * @param {number} baselineOffset
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>clip</code> property changes.
     * @function Stats.clipChanged
     * @param {boolean} clip
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>focus</code> property changes.
     * @function Stats.focusChanged
     * @param {boolean} focus
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>activeFocus</code> property changes.
     * @function Stats.activeFocusChanged
     * @param {boolean} activeFocus
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>activeFocusOnTab</code> property changes.
     * @function Stats.activeFocusOnTabChanged
     * @param {boolean} activeFocusOnTab
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>rotation</code> property changes.
     * @function Stats.rotationChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>scaleChanged</code> property changes.
     * @function Stats.scaleChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>transformOrigin</code> property changes.
     * @function Stats.transformOriginChanged
     * @param {number} transformOrigin
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>smooth</code> property changes.
     * @function Stats.smoothChanged
     * @param {boolean} smooth
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>antialiasing</code> property changes.
     * @function Stats.antialiasingChanged
     * @param {boolean} antialiasing
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>implicitWidth</code> property changes.
     * @function Stats.implicitWidthChanged
     * @returns {Signal}
     */

    /**jsdoc
     * Triggered when the value of the <code>implicitHeight</code> property changes.
     * @function Stats.implicitHeightChanged
     * @returns {Signal}
     */

     /**jsdoc
     * @function Stats.windowChanged
     * @param {object} window
     * @returns {Signal}
     */


    // QQuickItem functions.

    /**jsdoc
     * @function Stats.grabToImage
     * @param {object} callback
     * @param {Size} [targetSize=0,0]
     * @returns {boolean}
     */

    /**jsdoc
     * @function Stats.contains
     * @param {Vec2} point
     * @returns {boolean}
     */

    /**jsdoc
     * @function Stats.mapFromItem
     * @param {object} item
     */

    /**jsdoc
     * @function Stats.mapToItem
     * @param {object} item
     */

    /**jsdoc
     * @function Stats.mapFromGlobal
     * @param {object} global
     */

    /**jsdoc
     * @function Stats.mapToGlobal
     * @param {object} global
     */

    /**jsdoc
     * @function Stats.forceActiveFocus
     * @param {number} [reason=7]
     */

    /**jsdoc
     * @function Stats.nextItemInFocusChain
     * @param {boolean} [forward=true]
     * @returns {object}
     */

    /**jsdoc
     * @function Stats.childAt
     * @param {number} x
     * @param {number} y
     * @returns {object}
     */

    /**jsdoc
     * @function Stats.update
     */

    /**jsdoc
     * Triggered when the value of the <code>stylusPicksCount</code> property changes.
     * @function Stats.stylusPicksCountChanged
     * @returns {Signal}
     */
    void stylusPicksCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>rayPicksCount</code> property changes.
     * @function Stats.rayPicksCountChanged
     * @returns {Signal}
     */
    void rayPicksCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>parabolaPicksCount</code> property changes.
     * @function Stats.parabolaPicksCountChanged
     * @returns {Signal}
     */
    void parabolaPicksCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>collisionPicksCount</code> property changes.
     * @function Stats.collisionPicksCountChanged
     * @returns {Signal}
     */
    void collisionPicksCountChanged();

    /**jsdoc
     * Triggered when the value of the <code>stylusPicksUpdated</code> property changes.
     * @function Stats.stylusPicksUpdatedChanged
     * @returns {Signal}
     */
    void stylusPicksUpdatedChanged();

    /**jsdoc
     * Triggered when the value of the <code>rayPicksUpdated</code> property changes.
     * @function Stats.rayPicksUpdatedChanged
     * @returns {Signal}
     */
    void rayPicksUpdatedChanged();

    /**jsdoc
     * Triggered when the value of the <code>parabolaPicksUpdated</code> property changes.
     * @function Stats.parabolaPicksUpdatedChanged
     * @returns {Signal}
     */
    void parabolaPicksUpdatedChanged();

    /**jsdoc
     * Triggered when the value of the <code>collisionPicksUpdated</code> property changes.
     * @function Stats.collisionPicksUpdatedChanged
     * @returns {Signal}
     */
    void collisionPicksUpdatedChanged();

private:
    int _recentMaxPackets{ 0 } ; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon{ true };
    bool _expanded{ false };
    bool _showTimingDetails{ false };
    bool _showGameUpdateStats{ false };
    QString _monospaceFont;
    const AudioIOStats* _audioStats;
    QStringList _downloadUrls = QStringList();
};

#endif // hifi_Stats_h
