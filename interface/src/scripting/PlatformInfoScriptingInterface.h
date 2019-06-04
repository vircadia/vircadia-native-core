//
//  Created by Nissim Hadar on 2018/12/28
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PlatformInfoScriptingInterface_h
#define hifi_PlatformInfoScriptingInterface_h

#include <platform/Profiler.h>
#include <QtCore/QObject>

class QScriptValue;

/**jsdoc
 * The <code>PlatformInfo</code> API provides information about the computer and controllers being used.
 *
 * @namespace PlatformInfo
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class PlatformInfoScriptingInterface : public QObject {
    Q_OBJECT


public:
    PlatformInfoScriptingInterface();
    virtual ~PlatformInfoScriptingInterface();

    // Platform tier enum type
    enum PlatformTier {
        UNKNOWN = platform::Profiler::Tier::UNKNOWN,
        LOW = platform::Profiler::Tier::LOW,
        MID = platform::Profiler::Tier::MID,
        HIGH = platform::Profiler::Tier::HIGH,
    };
    Q_ENUM(PlatformTier);

public slots:
    /**jsdoc
     * @function PlatformInfo.getInstance
     * @deprecated This function is deprecated and will be removed.
     */
    static PlatformInfoScriptingInterface* getInstance();

    /**jsdoc
     * Gets the operating system type.
     * @function PlatformInfo.getOperatingSystemType
     * @returns {string} <code>"WINDOWS"</code>, <code>"MACOS"</code>, or <code>"UNKNOWN"</code>.
     */
    QString getOperatingSystemType();

    /**jsdoc
     * Gets information on the CPU.
     * @function PlatformInfo.getCPUBrand
     * @returns {string} Information on the CPU.
     * @example <caption>Report the CPU being used.</caption>
     * print("CPU: " + PlatformInfo.getCPUBrand());
     * // Example: Intel(R) Core(TM) i7-7820HK CPU @ 2.90GHz
     */
    QString getCPUBrand();

    /**jsdoc
     * Gets the number of logical CPU cores.
     * @function PlatformInfo.getNumLogicalCores
     * @returns {number} The number of logical CPU cores.
     */
    unsigned int getNumLogicalCores();

    /**jsdoc
     * Returns the total system memory in megabytes.
     * @function PlatformInfo.getTotalSystemMemoryMB
     * @returns {number} The total system memory in megabytes.
     */
    int getTotalSystemMemoryMB();

    /**jsdoc
     * Gets the graphics card type.
     * @function PlatformInfo.getGraphicsCardType
     * @returns {string} The graphics card type.
     */
    QString getGraphicsCardType();

    /**jsdoc
     * Checks whether Oculus Touch controllers are connected.
     * @function PlatformInfo.hasRiftControllers
     * @returns {boolean} <code>true</code> if Oculus Touch controllers are connected, <code>false</code> if they aren't.
     */
    bool hasRiftControllers();

    /**jsdoc
     * Checks whether Vive controllers are connected.
     * @function PlatformInfo.hasViveControllers
     * @returns {boolean} <code>true</code> if Vive controllers are connected, <code>false</code> if they aren't.
     */
    bool hasViveControllers();

    /**jsdoc
     * Checks whether HTML on 3D surfaces (e.g., Web entities) is supported.
     * @function PlatformInfo.has3DHTML
     * @returns {boolean} <code>true</code> if the current display supports HTML on 3D surfaces, <code>false</code> if it 
     * doesn't.
     */
    bool has3DHTML();

    /**jsdoc
     * Checks whether Interface is running on a stand-alone HMD device (CPU incorporated into the HMD display).
     * @function PlatformInfo.isStandalone
     * @returns {boolean} <code>true</code> if Interface is running on a stand-alone device, <code>false</code> if it isn't.
     */
    bool isStandalone();

    /**jsdoc
    * Get the number of CPUs.
    * @function PlatformInfo.getNumCPUs
    * @returns {number} The number of CPUs detected on the hardware platform.
    */
    int getNumCPUs();

    /**jsdoc
    * Get the description of the CPU at the index parameter
    * expected fields are:
    *  - cpuVendor...
    * @param index The index of the CPU of the platform
    * @function PlatformInfo.getCPU
    * @returns {string} The CPU description json field
    */
    QString getCPU(int index);

    /**jsdoc
     * Get the number of GPUs.
     * @function PlatformInfo.getNumGPUs
     * @returns {number} The number of GPUs detected on the hardware platform.
     */
    int getNumGPUs();

    /**jsdoc
     * Get the description of the GPU at the index parameter
     * expected fields are:
     *  - gpuVendor...
     * @param index The index of the GPU of the platform
     * @function PlatformInfo.getGPU
     * @returns {string} The GPU description json field
     */
    QString getGPU(int index);

    /**jsdoc
    * Get the number of Displays.
    * @function PlatformInfo.getNumDisplays
    * @returns {number} The number of Displays detected on the hardware platform.
    */
    int getNumDisplays();

    /**jsdoc
    * Get the description of the Display at the index parameter
    * expected fields are:
    *  - DisplayVendor...
    * @param index The index of the Display of the platform
    * @function PlatformInfo.getDisplay
    * @returns {string} The Display description json field
    */
    QString getDisplay(int index);

    /**jsdoc
    * Get the description of the Memory
    * expected fields are:
    *  - MemoryVendor...
    * @function PlatformInfo.getMemory
    * @returns {string} The Memory description json field
    */
    QString getMemory();

    /**jsdoc
    * Get the description of the Computer
    * expected fields are:
    *  - ComputerVendor...
    * @function PlatformInfo.getComputer
    * @returns {string} The Computer description json field
    */
    QString getComputer();


    /**jsdoc
    * Get the Platform TIer profiled on startup of the Computer
    * Platform Tier is an ineger/enum value:
    *  LOW = 0, MID = 1, HIGH = 2
    * @function PlatformInfo.getTierProfiled
    * @returns {number} The Platform Tier profiled on startup.
    */
    PlatformTier getTierProfiled();

    /**jsdoc
    * Get the Platform Tier possible Names as an array of strings 
    * Platform Tier is an ineger/enum value:
    *  LOW = 0, MID = 1, HIGH = 2
    * @function PlatformInfo.getPlatformTierNames
    * @returns {string} The array of names matching the number returned from PlatformInfo.getTierProfiled
    */
    QStringList getPlatformTierNames();


};

#endif  // hifi_PlatformInfoScriptingInterface_h
