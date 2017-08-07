// EnumMeta.js -- helper module that maps related enum values to printable names and ids

/* eslint-env commonjs */
/* global DriveKeys, console */

var VERSION = '0.0.1';
var WANT_DEBUG = false;

function _debugPrint() {
    // eslint-disable-next-line no-console
    (typeof Script === 'object' ? print : console.log)('EnumMeta | ' + [].slice.call(arguments).join(' '));
}

var debugPrint = WANT_DEBUG ? _debugPrint : function(){};

try {
    /* global process */
    if (process.title === 'node') {
        _defineNodeJSMocks();
    }
} catch (e) { /* eslint-disable-line empty-block */ }

_debugPrint(VERSION);

// FIXME: C++ emits this action event, but doesn't expose it yet to scripting
//   (ie: as Actions.ZOOM or Actions.TranslateCameraZ)
var ACTION_TRANSLATE_CAMERA_Z = {
    actionName: 'TranslateCameraZ',
    actionID: 12,
    driveKeyName: 'ZOOM'
};

module.exports = {
    version: VERSION,
    DriveKeyNames: invertKeys(DriveKeys),
    Controller: {
        Hardware: Object.keys(Controller.Hardware).reduce(function(names, prop) {
            names[prop+'Names'] = invertKeys(Controller.Hardware[prop]);
            return names;
        }, {}),
        ActionNames: _getActionNames(),
        StandardNames: invertKeys(Controller.Standard)
    },
    getDriveKeyNameFromActionName: getDriveKeyNameFromActionName,
    getActionNameFromDriveKeyName: getActionNameFromDriveKeyName,
    eventKeyText2KeyboardName: eventKeyText2KeyboardName,
    keyboardName2eventKeyText: keyboardName2eventKeyText,
    ACTION_TRANSLATE_CAMERA_Z: ACTION_TRANSLATE_CAMERA_Z,
    INVALID_ACTION_ID: Controller.findAction('INVALID_ACTION_ID_FOO')
};

_debugPrint('///'+VERSION, Object.keys(module.exports));

var actionsMapping = {}, driveKeyMapping = {};

initializeMappings(actionsMapping, driveKeyMapping);

function invertKeys(object) {
    if (!object) {
        return object;
    }
    return Object.keys(object).reduce(function(out, key) {
        out[object[key]] = key;
        return out;
    }, {});
}

function _getActionNames() {
    var ActionNames = invertKeys(Controller.Hardware.Actions);
    ActionNames[ACTION_TRANSLATE_CAMERA_Z.actionID] = ACTION_TRANSLATE_CAMERA_Z.actionName;
    function mapActionName(actionName) {
        var actionKey = Controller.Hardware.Actions[actionName],
            actionID = Controller.findAction(actionName),
            similarName = eventKeyText2KeyboardName(actionName),
            existingName = ActionNames[actionID];

        var keyName = actionName;

        if (actionID === module.exports.INVALID_ACTION_ID) {
            _debugPrint('actionID === INVALID_ACTION_ID', actionName);
        }
        switch (actionName) {
            case 'StepTranslateX': actionName = 'StepTranslate'; break;
            case 'StepTranslateY': actionName = 'StepTranslate'; break;
            case 'StepTranslateZ': actionName = 'StepTranslate'; break;
            case 'ACTION1': actionName = 'PrimaryAction'; break;
            case 'ACTION2': actionName = 'SecondaryAction'; break;
        }
        debugPrint(keyName, actionName, actionKey, actionID);

        similarName = similarName.replace('Lateral','Strafe').replace(/^(?:Longitudinal|Vertical)/, '');
        if (actionID in ActionNames) {
            // check if overlap is just BoomIn <=> BOOM_IN
            if (similarName !== existingName && actionName !== existingName) {
                throw new Error('assumption failed: overlapping actionID:'+JSON.stringify({
                    actionID: actionID,
                    actionKey: actionKey,
                    actionName: actionName,
                    similarName: similarName,
                    keyName: keyName,
                    existingName: existingName
                },0,2));
            }
        } else {
            ActionNames[actionID] = actionName;
            ActionNames[actionKey] = keyName;
        }
    }
    // first map non-legacy (eg: Up and not VERTICAL_UP) actions
    Object.keys(Controller.Hardware.Actions).filter(function(name) {
        return /[a-z]/.test(name);
    }).sort().reverse().forEach(mapActionName);
    // now legacy actions
    Object.keys(Controller.Hardware.Actions).filter(function(name) {
        return !/[a-z]/.test(name);
    }).sort().reverse().forEach(mapActionName);

    return ActionNames;
}

// attempts to brute-force translate an Action name into a DriveKey name
//   eg: _translateActionName('TranslateX') === 'TRANSLATE_X'
//   eg: _translateActionName('Yaw') === 'YAW'
function _translateActionName(name, _index) {
    name = name || '';
    var key = name;
    var re = new RegExp('[A-Z][a-z0-9]+', 'g');
    key = key.replace(re, function(Word) {
        return Word.toUpperCase()+'_';
    })
        .replace(/_$/, '');

    if (key in DriveKeys) {
        debugPrint('getDriveKeyFromEventName', _index, name, key, DriveKeys[key]);
        return key;
    }
}

function getActionNameFromDriveKeyName(driveKeyName) {
    return driveKeyMapping[driveKeyName];
}
// maps an action lookup value to a DriveKey name
//  eg: actionName: 'Yaw' === 'YAW'
//      actionKey:  Controller.Actions.Yaw => 'YAW'
//      actionID:   Controller.findAction('Yaw') => 'YAW'
function getDriveKeyNameFromActionName(lookupValue) {
    if (lookupValue === ACTION_TRANSLATE_CAMERA_Z.actionName ||
        lookupValue === ACTION_TRANSLATE_CAMERA_Z.actionID) {
        return ACTION_TRANSLATE_CAMERA_Z.driveKeyName;
    }
    if (typeof lookupValue === 'string') {
        lookupValue = Controller.findAction(lookupValue);
    }
    return actionsMapping[lookupValue];
}

// maps a Controller.key*Event event.text -> Controller.Hardware.Keyboard[name]
//   eg: ('Page Up') === 'PgUp'
//   eg: ('LEFT') === 'Left'
function eventKeyText2KeyboardName(text) {
    if (eventKeyText2KeyboardName[text]) {
        // use memoized value
        return eventKeyText2KeyboardName[text];
    }
    var keyboardName = (text||'').toUpperCase().split(/[ _]/).map(function(WORD) {
        return WORD.replace(/([A-Z])(\w*)/g, function(_, A, b) {
            return (A.toUpperCase() + b.toLowerCase());
        });
    }).join('').replace('Page','Pg');
    return eventKeyText2KeyboardName[text] = eventKeyText2KeyboardName[keyboardName] = keyboardName;
}

// maps a Controller.Hardware.Keyboard[name] -> Controller.key*Event event.text
//   eg: ('PgUp') === 'PAGE UP'
//   eg: ('Shift') === 'SHIFT'
function keyboardName2eventKeyText(keyName) {
    if (keyboardName2eventKeyText[keyName]) {
        // use memoized value
        return keyboardName2eventKeyText[keyName];
    }
    var text = keyName.replace('Pg', 'Page');
    var caseWords = text.match(/[A-Z][a-z0-9]+/g) || [ text ];
    var eventText = caseWords.map(function(str) {
        return str.toUpperCase();
    }).join('_');
    return keyboardName2eventKeyText[keyName] = eventText;
}

function initializeMappings(actionMap, driveKeyMap) {
    _debugPrint('creating mapping');
    var ref = ACTION_TRANSLATE_CAMERA_Z;
    actionMap[ref.actionName] = actionMap[ref.actionID] = ref.driveKeyName;
    actionMap.BoomIn = 'ZOOM';
    actionMap.BoomOut = 'ZOOM';

    Controller.getActionNames().sort().reduce(
        function(out, name, index, arr) {
            var actionKey = arr[index];
            var actionID = Controller.findAction(name);
            var value = actionID in out ? out[actionID] : _translateActionName(name, index);
            if (value !== undefined) {
                var prefix = (actionID in out ? '+++' : '---');
                debugPrint(prefix + ' Action2DriveKeyName['+name+'('+actionID+')] = ' + value);
                driveKeyMap[value] = driveKeyMap[value] || name;
            }
            out[name] = out[actionID] = out[actionKey] = value;
            return out;
        }, actionMap);
}

// ----------------------------------------------------------------------------
// mocks for litmus testing using Node.js command line tools
function _defineNodeJSMocks() {
    /* eslint-disable no-global-assign */
    DriveKeys = {
        TRANSLATE_X: 12345
    };
    Controller = {
        getActionNames: function() {
            return Object.keys(this.Hardware.Actions);
        },
        findAction: function(name) {
            return this.Hardware.Actions[name] || 4095;
        },
        Hardware: {
            Actions: {
                TranslateX: 54321
            },
            Application: {
                Grounded: 1
            },
            Keyboard: {
                A: 65
            }
        }
    };
    /* eslint-enable no-global-assign */
}
