//
//  feedback.js
//
//  Created by David Rowe on 31 Aug 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Feedback:true */

Feedback = (function () {
    // Provide audio and haptic user feedback. 
    // Global object.

    "use strict";

    var DROP_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/drop.wav")),
        DELETE_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/delete.wav")),
        SELECT_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/select.wav")),
        CLONE_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/clone.wav")),
        CREATE_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/create.wav")),
        EQUIP_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/equip.wav")),
        ERROR_SOUND = SoundCache.getSound(Script.resolvePath("../assets/audio/error.wav")),
        UNDO_SOUND = DROP_SOUND,
        REDO_SOUND = DROP_SOUND,

        FEEDBACK_PARAMETERS = {
            DROP_TOOL:       { sound: DROP_SOUND, volume: 0.3, haptic: 0.75 },
            DELETE_ENTITY:   { sound: DELETE_SOUND, volume: 0.5, haptic: 0.2 },
            SELECT_ENTITY:   { sound: SELECT_SOUND, volume: 0.2, haptic: 0.1 }, // E.g., Group tool.
            CLONE_ENTITY:    { sound: CLONE_SOUND, volume: 0.2, haptic: 0.1 },
            CREATE_ENTITY:   { sound: CREATE_SOUND, volume: 0.4, haptic: 0.2 },
            HOVER_MENU_ITEM: { sound: null, volume: 0, haptic: 0.1 }, // Tools menu.
            HOVER_BUTTON:    { sound: null, volume: 0, haptic: 0.075 }, // Tools options and Create palette items.
            EQUIP_TOOL:      { sound: EQUIP_SOUND, volume: 0.3, haptic: 0.6 },
            APPLY_PROPERTY:  { sound: null, volume: 0, haptic: 0.3 },
            APPLY_ERROR:     { sound: ERROR_SOUND, volume: 0.2, haptic: 0.7 },
            UNDO_ACTION:     { sound: UNDO_SOUND, volume: 0.1, haptic: 0.2 },
            REDO_ACTION:     { sound: REDO_SOUND, volume: 0.1, haptic: 0.2 },
            GENERAL_ERROR:   { sound: ERROR_SOUND, volume: 0.2, haptic: 0.7 }
        },

        VOLUME_MULTPLIER = 0.5, // Resulting volume range should be within 0.0 - 1.0.
        HAPTIC_STRENGTH_MULTIPLIER = 1.3, // Resulting strength range should be within 0.0 - 1.0.
        HAPTIC_LENGTH_MULTIPLIER = 75.0; // Resulting length range should be within 0 - 50, say.

    function play(side, item) {
        var parameters = FEEDBACK_PARAMETERS[item];

        if (parameters.sound) {
            Audio.playSound(parameters.sound, {
                position: side ? MyAvatar.getRightPalmPosition() : MyAvatar.getLeftPalmPosition(),
                volume: parameters.volume * VOLUME_MULTPLIER,
                localOnly: true
            });
        }

        Controller.triggerHapticPulse(parameters.haptic * HAPTIC_STRENGTH_MULTIPLIER,
            parameters.haptic * HAPTIC_LENGTH_MULTIPLIER, side);
    }

    return {
        DROP_TOOL: "DROP_TOOL",
        DELETE_ENTITY: "DELETE_ENTITY",
        SELECT_ENTITY: "SELECT_ENTITY",
        CLONE_ENTITY: "CLONE_ENTITY",
        CREATE_ENTITY: "CREATE_ENTITY",
        HOVER_MENU_ITEM: "HOVER_MENU_ITEM",
        HOVER_BUTTON: "HOVER_BUTTON",
        EQUIP_TOOL: "EQUIP_TOOL",
        APPLY_PROPERTY: "APPLY_PROPERTY",
        APPLY_ERROR: "APPLY_ERROR",
        UNDO_ACTION: "UNDO_ACTION",
        REDO_ACTION: "REDO_ACTION",
        GENERAL_ERROR: "GENERAL_ERROR",
        play: play
    };
}());
