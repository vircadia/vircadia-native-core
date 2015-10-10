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

#include <string>
#include <vector>
#include <map>

// Singleton template class
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
class DeviceTracker {
public:

    // THe ID and Name types used to manage the pool of devices
    typedef std::string Name;
    typedef int ID;
    static const ID INVALID_DEVICE = -1;
    static const ID INVALID_DEVICE_NAME = -2;

    // Singleton interface to register and query the devices currently connected
    static int getNumDevices();
    static ID getDeviceID(const Name& name);
    static DeviceTracker* getDevice(ID deviceID);
    static DeviceTracker* getDevice(const Name& name);

    /// Update all the devices calling for their update() function
    /// This should be called every frame by the main loop to update all the devices that pull their state
    static void updateAll();

    /// Register a device tracker to the factory
    /// Right after creating a new DeviceTracker, it should be registered
    /// This is why, it's recommended to use a factory static call in the specialized class
    /// to create a new input device
    ///
    /// \param name     The Name under wich registering the device
    /// \param parent   The DeviceTracker
    ///
    /// \return The Index of the newly registered device.
    ///         Valid if everything went well.
    ///         INVALID_DEVICE if the device is not valid (NULL)
    ///         INVALID_DEVICE_NAME if the name is already taken
    static ID registerDevice(const Name& name, DeviceTracker* tracker);

    static void destroyDevice(const Name& name);

    // DeviceTracker interface

    virtual void update();

    /// Get the ID assigned to the Device when registered after creation, or INVALID_DEVICE if it hasn't been registered which should not happen.
    ID getID() const { return _ID; }

    /// Get the name assigned to the Device when registered after creation, or "Unknown" if it hasn't been registered which should not happen.
    const Name& getName() const { return _name; }

    typedef std::map< Name, ID > Map;
    static const Map& getDevices() { return Singleton::get()->_devicesMap; }

protected:
    DeviceTracker();
    virtual ~DeviceTracker();

private:
    ID _ID;
    Name _name;

    // this call is used by the singleton when the device tracker is currently beeing registered and beeing assigned an ID
    void assignIDAndName( const ID id, const Name& name ) { _ID = id; _name = name; }

    typedef std::vector< DeviceTracker* > Vector;
    struct SingletonData {
        Map     _devicesMap;
        Vector  _devicesVector;

        ~SingletonData();
    };
    typedef TemplateSingleton< SingletonData > Singleton;
};

#endif // hifi_DeviceTracker_h
