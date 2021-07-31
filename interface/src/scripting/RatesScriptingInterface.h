//
//  RatesScriptingInterface.h 
//  interface/src/scripting
//
//  Created by Zach Pomerantz on 4/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef HIFI_RATES_SCRIPTING_INTERFACE_H
#define HIFI_RATES_SCRIPTING_INTERFACE_H

#include <display-plugins/DisplayPlugin.h>

/*@jsdoc
 *  The <code>Rates</code> API provides some information on current rendering performance.
 *
 * @namespace Rates
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {number} render - The rate at which new GPU frames are being created, in Hz.
 *     <em>Read-only.</em>
 * @property {number} present - The rate at which the display plugin is presenting to the display device, in Hz.
 *     <em>Read-only.</em>
 * @property {number} newFrame - The rate at which the display plugin is presenting new GPU frames, in Hz.
 *     <em>Read-only.</em>
 * @property {number} dropped - The rate at which the display plugin is dropping GPU frames, in Hz.
 *     <em>Read-only.</em>
 * @property {number} simulation - The rate at which the game loop is running, in Hz.
 *     <em>Read-only.</em>
 *
 * @example <caption>Report current rendering rates.</caption>
 * // The rates to report.
 * var rates = [
 *     "render",
 *     "present",
 *     "newFrame",
 *     "dropped",
 *     "simulation"
 * ];
 * 
 * // Report the rates.
 * for (var i = 0; i < rates.length; i++) {
 *     print("Rates." + rates[i], "=", Rates[rates[i]]);
 * }
 */
class RatesScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(float render READ getRenderRate)
    Q_PROPERTY(float present READ getPresentRate)
    Q_PROPERTY(float newFrame READ getNewFrameRate)
    Q_PROPERTY(float dropped READ getDropRate)
    Q_PROPERTY(float simulation READ getSimulationRate)

public:
    RatesScriptingInterface(QObject* parent) : QObject(parent) {}
    float getRenderRate() { return qApp->getRenderLoopRate(); }
    float getPresentRate() { return qApp->getActiveDisplayPlugin()->presentRate(); }
    float getNewFrameRate() { return qApp->getActiveDisplayPlugin()->newFramePresentRate(); }
    float getDropRate() { return qApp->getActiveDisplayPlugin()->droppedFrameRate(); }
    float getSimulationRate() { return qApp->getGameLoopRate(); }
};

#endif // HIFI_INTERFACE_RATES_SCRIPTING_INTERFACE_H
