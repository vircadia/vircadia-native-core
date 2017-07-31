//  midiSphere.js
//
//  Script Type: Entity
//  Created by James B. Pollack @imgntn on 9/21/2015
//  Adapted by Burt
//  Copyright 2015 High Fidelity, Inc.
//  
//  This script listens to MIDI and makes the ball change color.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;

    function MidiSphere() {
        _this = this;
        this.clicked = false;
        return;
    }
    
    MidiSphere.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            Midi.midiNote.connect(function(eventData) {
                print("MidiSphere.noteReceived: "+JSON.stringify(eventData));
                Entities.editEntity(entityID, { color: { red: 2*eventData.note, green: 2*eventData.note, blue: 2*eventData.note} });
            });
            print("MidiSphere.preload");
        },
        unload: function(entityID) {
            print("MidiSphere.unload");
        },

        clickDownOnEntity: function(entityID, mouseEvent) { 
            print("MidiSphere.clickDownOnEntity");
            if (this.clicked) {
                Entities.editEntity(entityID, { color: { red: 0, green: 255, blue: 255} });
                this.clicked = false;
                Midi.playMidiNote(144, 64, 0);
            } else {
                Entities.editEntity(entityID, { color: { red: 255, green: 255, blue: 0} });
                this.clicked = true;
                Midi.playMidiNote(144, 64, 100);
            }
        }

    };

    // entity scripts should return a newly constructed object of our type
    return new MidiSphere();
});