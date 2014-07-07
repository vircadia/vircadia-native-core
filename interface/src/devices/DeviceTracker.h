//
//  DeviceTracker.h
//  interface/src/devices
//
//  Created by Sam Cake on 6/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DeviceTracker_h
#define hifi_DeviceTracker_h

#include <QObject>
#include <QVector>

//--------------------------------------------------------------------------------------
// Singleton template class
//--------------------------------------------------------------------------------------
template < typename T >
class TemplateSingleton {
public:

    static T* get() {
        if ( !_singleton._one ) {
            _singleton._one = new T();
        }
        return _singleton._one;
    }

    TemplateSingleton() :
        _one(0)
        {
        }
    ~TemplateSingleton() {
        if ( _one ) {
            delete _one;
            _one = 0;
        }
    }
private:
    static TemplateSingleton< T > _singleton;
    T* _one;
};
template <typename T>
TemplateSingleton<T> TemplateSingleton<T>::_singleton;

/// Base class for device trackers.
class DeviceTracker : public QObject {
    Q_OBJECT
public:

    typedef std::string Name;
    typedef qint64 Stamp;

    // Singleton interface to register and query the Devices currently connected

    static int init();
    static int getNumDevices();
    static int getDeviceIndex( const Name& name );
    static DeviceTracker* getDevice( int deviceNum );
    static DeviceTracker* getDevice( const Name& name );
    
    /// Update all the devices calling for their update() function
    static void updateAll();

    static int registerDevice( const Name& name, DeviceTracker* tracker );

    // DeviceTracker interface
    virtual bool isConnected() const;
    virtual void update();

protected:
    DeviceTracker();
    virtual ~DeviceTracker();

private:

    struct SingletonData {
        typedef std::map< Name, int > Map;
        typedef std::vector< DeviceTracker* > Vector;
        Map     _devicesMap;
        Vector  _devicesVector;
    };
    typedef TemplateSingleton< SingletonData > Singleton;
};

#endif // hifi_DeviceTracker_h
