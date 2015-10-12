//
//  controllerScriptingExamples.js
//  examples
//
//  Created by Sam Gondelman on 6/2/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Assumes you only have the default keyboard connected


Object.keys(Controller.Standard).forEach(function (input) {
    print("Controller.Standard." + input + ":" + Controller.Standard[input]);
});

Object.keys(Controller.Hardware).forEach(function (deviceName) {
    Object.keys(Controller.Hardware[deviceName]).forEach(function (input) {
        print("Controller.Hardware." + deviceName + "." + input + ":" + Controller.Hardware[deviceName][input]);
    });
});

Object.keys(Controller.Actions).forEach(function (actionName) {
    print("Controller.Actions." + actionName + ":" + Controller.Actions[actionName]);
});

// Resets every device to its default key bindings:
Controller.resetAllDeviceBindings();

// Query all actions
print("All Actions: \n" + Controller.getAllActions());

// Each action stores:
// action: int representation of enum
print("Action 5 int: \n" + Controller.getAllActions()[5].action);

// actionName: string representation of enum
print("Action 5 name: \n" + Controller.getAllActions()[5].actionName);

// inputChannels: list of all inputchannels that control that action
print("Action 5 input channels: \n" + Controller.getAllActions()[5].inputChannels + "\n");


// Each input channel stores:
// action: Action that this InputChannel maps to
print("Input channel action: \n" + Controller.getAllActions()[5].inputChannels[0].action);

// scale: sensitivity of input
print("Input channel scale: \n" + Controller.getAllActions()[5].inputChannels[0].scale);

// input and modifier: Inputs
print("Input channel input and modifier: \n" + Controller.getAllActions()[5].inputChannels[0].input + "\n" + Controller.getAllActions()[5].inputChannels[0].modifier + "\n");


// Each Input stores:
// device: device of input
print("Input device: \n" + Controller.getAllActions()[5].inputChannels[0].input.device);

// channel: channel of input
print("Input channel: \n" + Controller.getAllActions()[5].inputChannels[0].input.channel);

// type: type of input (Unknown, Button, Axis, Joint)
print("Input type: \n" + Controller.getAllActions()[5].inputChannels[0].input.type);

// id: id of input
print("Input id: \n" + Controller.getAllActions()[5].inputChannels[0].input.id + "\n");


// You can get the name of a device from its id
print("Device 1 name: \n" + Controller.getDeviceName(Controller.getAllActions()[5].inputChannels[0].input.id));

// You can also get all of a devices input channels
print("Device 1's input channels: \n" + Controller.getAllInputsForDevice(1) + "\n");


// Modifying properties:
// The following code will switch the "w" and "s" key functionality and adjust their scales
var s = Controller.getAllActions()[0].inputChannels[0];
var w = Controller.getAllActions()[1].inputChannels[0];

// You must remove an input controller before modifying it so the old input controller isn't registered anymore
// removeInputChannel and addInputChannel return true if successful, false otherwise
Controller.removeInputChannel(s);
Controller.removeInputChannel(w);
print(s.scale);
s.action = 1;
s.scale = .01;

w.action = 0;
w.scale = 10000;
Controller.addInputChannel(s);
Controller.addInputChannel(w);
print(s.scale);

// You can get all the available inputs for any device
// Each AvailableInput has:
// input: the Input itself
// inputName: string representing the input
var availableInputs = Controller.getAvailableInputs(1);
for (i = 0; i < availableInputs.length; i++) {
    print(availableInputs[i].inputName);
}

// You can modify key bindings by using these avaiable inputs
// This will replace e (up) with 6
var e = Controller.getAllActions()[5].inputChannels[0];
Controller.removeInputChannel(e);
e.input = availableInputs[6].input;
Controller.addInputChannel(e);