//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QString>
#include <QObject>

#include "Forward.h"

class Plugin : public QObject {
public:
    virtual const QString& getName() const = 0;
    virtual bool isSupported() const;
    
    static void setContainer(PluginContainer* container);

    /// Called when plugin is initially loaded, typically at application start
    virtual void init();

    /// Called when application is shutting down
    virtual void deinit();

    /// Called when a plugin is being activated for use.  May be called multiple times.
    virtual void activate() = 0;
    /// Called when a plugin is no longer being used.  May be called multiple times.
    virtual void deactivate() = 0;

    /**
     * Called by the application during it's idle phase.  If the plugin needs to do
     * CPU intensive work, it should launch a thread for that, rather than trying to
     * do long operations in the idle call
     */
    virtual void idle();

protected:
    static PluginContainer* CONTAINER;
};
