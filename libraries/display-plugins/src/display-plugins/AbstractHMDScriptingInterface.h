//
//  Created by Bradley Austin Davis on 2015/10/04
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AbstractHMDScriptingInterface_h
#define hifi_AbstractHMDScriptingInterface_h

#include <GLMHelpers.h>

// These properties have JSDoc documentation in HMDScriptingInterface.h.
class AbstractHMDScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ isHMDMode NOTIFY displayModeChanged)
    Q_PROPERTY(float ipd READ getIPD)
    Q_PROPERTY(float eyeHeight READ getEyeHeight)
    Q_PROPERTY(float playerHeight READ getPlayerHeight)
    Q_PROPERTY(float ipdScale READ getIPDScale WRITE setIPDScale NOTIFY IPDScaleChanged)
    Q_PROPERTY(bool mounted READ isMounted NOTIFY mountedChanged)

public:
    AbstractHMDScriptingInterface();
    float getIPD() const;
    float getEyeHeight() const;
    float getPlayerHeight() const;
    float getIPDScale() const;
    void setIPDScale(float ipdScale);
    bool isHMDMode() const;
    virtual bool isMounted() const = 0;

signals:
    /*@jsdoc
     * Triggered when the <code>HMD.ipdScale</code> property value changes.
     * @function HMD.IPDScaleChanged
     * @returns {Signal}
     */
    void IPDScaleChanged();

    /*@jsdoc
     * Triggered when Interface's display mode changes and when the user puts on or takes off their HMD.
     * @function HMD.displayModeChanged
     * @param {boolean} isHMDMode - <code>true</code> if the display mode is HMD, otherwise <code>false</code>. This is the
     *     same value as provided by <code>HMD.active</code>.
     * @returns {Signal}
     * @example <caption>Report when the display mode changes.</caption>
     * HMD.displayModeChanged.connect(function (isHMDMode) {
     *     print("Display mode changed");
     *     print("isHMD = " + isHMDMode);
     *     print("HMD.active = " + HMD.active);
     *     print("HMD.mounted = " + HMD.mounted);
     * });
     */
    void displayModeChanged(bool isHMDMode);

    /*@jsdoc
     * Triggered when the <code>HMD.mounted</code> property value changes.
     * @function HMD.mountedChanged
     * @returns {Signal}
     * @example <caption>Report when there's a change in the HMD being worn.</caption>
     * HMD.mountedChanged.connect(function () {
     *     print("Mounted changed. HMD is mounted: " + HMD.mounted);
     * });
     */
    void mountedChanged();

private:
    float _IPDScale{ 1.0 };
};

#endif // hifi_AbstractHMDScriptingInterface_h
