//
//  Created by Bradley Austin Davis on 2019/05/14
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_PerformanceScriptingInterface_h
#define hifi_PerformanceScriptingInterface_h

#include <mutex>

#include <QObject>

#include "../PerformanceManager.h"
#include "../RefreshRateManager.h"


/*@jsdoc
 * The <code>Performance</code> API provides control and information on graphics performance settings.
 *
 * @namespace Performance
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {Performance.PerformancePreset} performancePreset - The current graphics performance preset.
 * @property {Performance.RefreshRateProfile} refreshRateProfile - The current refresh rate profile.
 */
class PerformanceScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(PerformancePreset performancePreset READ getPerformancePreset WRITE setPerformancePreset NOTIFY settingsChanged)
    Q_PROPERTY(RefreshRateProfile refreshRateProfile READ getRefreshRateProfile WRITE setRefreshRateProfile NOTIFY settingsChanged)

public:

    /*@jsdoc
     * <p>Graphics performance presets.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>UNKNOWN</td><td>Custom settings of world detail, rendering effects, and refresh 
     *       rate.</td></tr>
     *     <tr><td><code>1</code></td><td>LOW</td><td>Low world detail, no rendering effects, lo refresh rate.</td></tr>
     *     <tr><td><code>2</code></td><td>MID</td><td>Medium world detail, some rendering effects, medium refresh 
     *       rate.</td></tr>
     *     <tr><td><code>3</code></td><td>HIGH</td><td>Maximum world detail, all rendering effects, high refresh rate.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} Performance.PerformancePreset
     */
    // PerformanceManager PerformancePreset tri state level enums
    enum PerformancePreset {
        UNKNOWN = PerformanceManager::PerformancePreset::UNKNOWN,
        LOW_POWER = PerformanceManager::PerformancePreset::LOW_POWER,
        LOW = PerformanceManager::PerformancePreset::LOW,
        MID = PerformanceManager::PerformancePreset::MID,
        HIGH = PerformanceManager::PerformancePreset::HIGH,
    };
    Q_ENUM(PerformancePreset)

    /*@jsdoc
     * <p>Refresh rate profile.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>ECO</td><td>Low refresh rate, which is reduced when Interface doesn't have focus or 
     *       is minimized.</td></tr>
     *     <tr><td><code>1</code></td><td>INTERACTIVE</td><td>Medium refresh rate, which is reduced when Interface doesn't have 
     *       focus or is minimized.</td></tr>
     *     <tr><td><code>2</code></td><td>REALTIME</td><td>High refresh rate, even when Interface doesn't have focus or is 
     *       minimized. </td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} Performance.RefreshRateProfile
     */
    // Must match RefreshRateManager enums
    enum RefreshRateProfile {
        ECO = RefreshRateManager::RefreshRateProfile::ECO,
        INTERACTIVE = RefreshRateManager::RefreshRateProfile::INTERACTIVE,
        REALTIME = RefreshRateManager::RefreshRateProfile::REALTIME,
    };
    Q_ENUM(RefreshRateProfile)

    PerformanceScriptingInterface();
    ~PerformanceScriptingInterface() = default;

public slots:

    /*@jsdoc
     * Sets graphics performance to a preset.
     * @function Performance.setPerformancePreset
     * @param {Performance.PerformancePreset} performancePreset - The graphics performance preset to to use.
     */
    void setPerformancePreset(PerformancePreset performancePreset);

    /*@jsdoc
     * Gets the current graphics performance preset in use.
     * @function Performance.getPerformancePreset
     * @returns {Performance.PerformancePreset} The current graphics performance preset in use.
     */
    PerformancePreset getPerformancePreset() const;

    /*@jsdoc
     * Gets the names of the graphics performance presets.
     * @function Performance.getPerformancePresetNames
     * @returns {string[]} The names of the graphics performance presets. The array index values correspond to 
     *     {@link Performance.PerformancePreset} values.
     */
    QStringList getPerformancePresetNames() const;


    /*@jsdoc
     * Sets the curfrent refresh rate profile.
     * @function Performance.setRefreshRateProfile
     * @param {Performance.RefreshRateProfile} refreshRateProfile - The refresh rate profile.
     */
    void setRefreshRateProfile(RefreshRateProfile refreshRateProfile);

    /*@jsdoc
     * Gets the current refresh rate profile in use.
     * @function Performance.getRefreshRateProfile
     * @returns {Performance.RefreshRateProfile} The refresh rate profile.
     */
    RefreshRateProfile getRefreshRateProfile() const;

    /*@jsdoc
     * Gets the names of the refresh rate profiles.
     * @function Performance.getRefreshRateProfileNames
     * @returns {string[]} The names of the refresh rate profiles. The array index values correspond to 
     *     {@link Performance.RefreshRateProfile} values.
     */
    QStringList getRefreshRateProfileNames() const;


    /*@jsdoc
     * Gets the current target refresh rate, in Hz, per the current refresh rate profile and refresh rate regime if in desktop 
     * mode; a higher rate if in VR mode.
     * @function Performance.getActiveRefreshRate
     * @returns {number} The current target refresh rate, in Hz.
     */
    int getActiveRefreshRate() const;

    /*@jsdoc
     * Gets the current user experience mode.
     * @function Performance.getUXMode
     * @returns {UXMode} The current user experience mode.
     */
    RefreshRateManager::UXMode getUXMode() const;

    /*@jsdoc
     * Gets the current refresh rate regime that's in effect.
     * @function Performance.getRefreshRateRegime
     * @returns {RefreshRateRegime} The current refresh rate regime.
     */
    RefreshRateManager::RefreshRateRegime getRefreshRateRegime() const;

signals:

    /*@jsdoc
     * Triggered when the performance preset or refresh rate profile is changed.
     * @function Performance.settingsChanged
     * @returns {Signal}
     */
    void settingsChanged();

private:
    static std::once_flag registry_flag;
};

#endif // header guard
