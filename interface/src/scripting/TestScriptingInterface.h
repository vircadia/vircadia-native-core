//
//  Created by Bradley Austin Davis on 2016/12/12
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_TestScriptingInterface_h
#define hifi_TestScriptingInterface_h

#include <QtCore/QObject>

class TestScriptingInterface : public QObject {
    Q_OBJECT

public slots:
    static TestScriptingInterface* getInstance();

    /**jsdoc
     * Exits the application
     */
    void quit();

    /**jsdoc
    * Waits for all texture transfers to be complete
    */
    void waitForTextureIdle();

    /**jsdoc
    * Waits for all pending downloads to be complete
    */
    void waitForDownloadIdle();

    /**jsdoc
    * Waits for all pending downloads and texture transfers to be complete
    */
    void waitIdle();

};

#endif // hifi_TestScriptingInterface_h
