//
//  Created by Bradley Austin Davis on 2015/10/18
//  (based on UserInputMapper inner class created by Sam Gateau on 4/27/15)
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_controllers_Input_h
#define hifi_controllers_Input_h

#include <GLMHelpers.h>

namespace controller {

enum class ChannelType {
    UNKNOWN = 0,
    BUTTON = 1,
    AXIS,
    POSE,
};

// Input is the unique identifier to find a n input channel of a particular device
// Devices are responsible for registering to the UseInputMapper so their input channels can be sued and mapped
// to the Action channels
struct Input {
    union {
        struct {
            uint16_t _device; // Up to 64K possible devices
            uint16_t _channel : 13; // 2^13 possible channel per Device
            uint16_t _type : 2; // 2 bits to store the Type directly in the ID
            uint16_t _padding : 1; // 2 bits to store the Type directly in the ID
        };
        uint32_t _id = 0; // by default Input is 0 meaning invalid
    };

    bool isValid() const { return (_id != 0); }

    uint16_t getDevice() const { return _device; }
    uint16_t getChannel() const { return _channel; }
    uint32_t getID() const { return _id; }
    ChannelType getType() const { return (ChannelType) _type; }

    void setDevice(uint16_t device) { _device = device; }
    void setChannel(uint16_t channel) { _channel = channel; }
    void setType(uint16_t type) { _type = type; }
    void setID(uint32_t ID) { _id = ID; }

    bool isButton() const { return getType() == ChannelType::BUTTON; }
    bool isAxis() const { return getType() == ChannelType::AXIS; }
    bool isPose() const { return getType() == ChannelType::POSE; }

    // WORKAROUND: the explicit initializer here avoids a bug in GCC-4.8.2 (but not found in 4.9.2)
    // where the default initializer (a C++-11ism) for the union data above is not applied.
    explicit Input() : _id(0) {}
    explicit Input(uint32_t id) : _id(id) {}
    explicit Input(uint16_t device, uint16_t channel, ChannelType type) : _device(device), _channel(channel), _type(uint16_t(type)), _padding(0) {}
    Input(const Input& src) : _id(src._id) {}
    Input& operator = (const Input& src) { _id = src._id; return (*this); }
    bool operator ==(const Input& right) const { return _id == right._id; }
    bool operator < (const Input& src) const { return _id < src._id; }

    static const Input INVALID_INPUT;
    static const uint16_t INVALID_DEVICE;
    static const uint16_t INVALID_CHANNEL;
    static const uint16_t INVALID_TYPE;

    using NamedPair = QPair<Input, QString>;
    using NamedVector = QVector<NamedPair>;
};

}

#endif
