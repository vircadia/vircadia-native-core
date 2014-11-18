const KBD_UPPERCASE_DEFAULT = 0;
const KBD_LOWERCASE_DEFAULT = 1;
const KBD_UPPERCASE_HOVER   = 2;
const KBD_LOWERCASE_HOVER   = 3;
const KBD_BACKGROUND        = 4;

const KEYBOARD_URL = "http://test.thoys.nl/hifi/images/virtualKeyboard/keyboard.svg";

const KEYBOARD_HEIGHT = 434.1;
const KEYBOARD_WIDTH  = 1174.7;

const BOUND_X = 0;
const BOUND_Y = 1;
const BOUND_W = 2;
const BOUND_H = 3;

const KEY_STATE_LOWER = 0;
const KEY_STATE_UPPER = 1;

function KeyboardKey(keyboard, key_properties) {
    this.event = key_properties.event != undefined ?
        key_properties.event : 'keypress';
    this.bounds = key_properties.bounds;
    this.states = key_properties.states;
    this.keyboard = keyboard;
    this.key_state = key_properties.key_state != undefined ? key_properties.key_state : KBD_LOWERCASE_HOVER;
    // one overlay per bound vector [this.bounds]
    this.overlays = [];
    this.updatePosition = function() {
    };
    this.containsCoord = function(x, y) {
        for (var i = 0; i < this.bounds.length; i++) {
             if (x >= this.bounds[i][BOUND_X] &&
                 x <= (this.bounds[i][BOUND_X] + this.bounds[i][BOUND_W]) &&
                 y >= this.bounds[i][BOUND_Y] &&
                 y <= (this.bounds[i][BOUND_Y] + this.bounds[i][BOUND_H]))
             {
                 return true;
             }
        }
        return false;
    };
    this.remove = function() {
        for (var i = 0; i < this.overlays.length; i++) {
             Overlays.deleteOverlay(this.overlays[i]);
        }
    };
    this.isLoaded = function() {
        for (var i = 0; i < this.overlays.length; i++) {
             if (!Overlays.isLoaded(this.overlays[i])) {
                return false;
             }
        }
        return true;
    };
    for (var i = 0; i < this.bounds.length; i++) {
        var newOverlay = Overlays.cloneOverlay(this.keyboard.background);
        Overlays.editOverlay(newOverlay, {
            x: 50 + this.bounds[i][BOUND_X],
            y: 50 + this.bounds[i][BOUND_Y],
            width: this.bounds[i][BOUND_W],
            height: this.bounds[i][BOUND_H],
            subImage: {width: this.bounds[i][BOUND_W], height: this.bounds[i][BOUND_H], x: this.bounds[i][BOUND_X], y: (KEYBOARD_HEIGHT * this.key_state) + this.bounds[i][BOUND_Y]},
            alpha: 1
        });
        this.overlays.push(newOverlay);
    }
}

function Keyboard() {
    this.background = Overlays.addOverlay("image", {
        x: 50,
        y: 50,
        width: KEYBOARD_WIDTH,
        height: KEYBOARD_HEIGHT,
        subImage: {width: KEYBOARD_WIDTH, height: KEYBOARD_HEIGHT, y: KEYBOARD_HEIGHT * KBD_BACKGROUND},
        imageURL: KEYBOARD_URL,
        alpha: 1
    });
    this.updatePosition = function() {
        
    };
    this.getFocussedKey = function() {
        
    };
    this.remove = function() {
        Overlays.deleteOverlay(this.background);
        for (var i = 0; i < this.keys.length; i++) {
            this.keys[i].remove();
        }
    };

    this.onKeyPress = null;
    this.onKeyDown = null;
    this.onKeyUp = null;
    this.onSubmit = null;

    this.keys = [];
    //
    // keyProperties contains the key data
    //
    // coords [[x,y,w,h],[x,y,w,h]]
    // states array of 1 or 2 objects [lowercase, uppercase] each object contains a charCode and a char
    var keyProperties = [
        {bounds: [[12, 12, 65, 52]], states: [{charCode: 192, char: '~'}]},
        {bounds: [[84, 12, 65, 52]], states: [{charCode: 192, char: '!'}]},
        {bounds: [[156, 12, 65, 52]], states: [{charCode: 192, char: '@'}]},
        {bounds: [[228, 12, 65, 52]], states: [{charCode: 192, char: '#'}]},
        {bounds: [[300, 12, 65, 52]], states: [{charCode: 192, char: '$'}]},
        {bounds: [[372, 12, 65, 52]], states: [{charCode: 192, char: '%'}]},
        {bounds: [[445, 12, 65, 52]], states: [{charCode: 192, char: '^'}]},
        {bounds: [[517, 12, 65, 52]], states: [{charCode: 192, char: '&'}]},
        {bounds: [[589, 12, 65, 52]], states: [{charCode: 192, char: '*'}]},
        {bounds: [[662, 12, 65, 52]], states: [{charCode: 192, char: '('}]},
        {bounds: [[734, 12, 65, 52]], states: [{charCode: 192, char: ')'}]},
        {bounds: [[806, 12, 65, 52]], states: [{charCode: 192, char: '_'}]},
        {bounds: [[881, 12, 65, 52]], states: [{charCode: 192, char: '{'}]},
        {bounds: [[953, 12, 65, 52]], states: [{charCode: 192, char: '}'}]},
        {bounds: [[1025, 12, 65, 52]], states: [{charCode: 192, char: '<'}]},
        {bounds: [[1097, 12, 65, 52]], states: [{charCode: 192, char: '>'}]},

        {bounds: [[12, 71, 65, 63]], states: [{charCode: 192, char: '`'}]},
        {bounds: [[84, 71, 65, 63]], states: [{charCode: 192, char: '1'}]},
        {bounds: [[156, 71, 65, 63]], states: [{charCode: 192, char: '2'}]},
        {bounds: [[228, 71, 65, 63]], states: [{charCode: 192, char: '3'}]},
        {bounds: [[300, 71, 65, 63]], states: [{charCode: 192, char: '4'}]},
        {bounds: [[372, 71, 65, 63]], states: [{charCode: 192, char: '5'}]},
        {bounds: [[445, 71, 65, 63]], states: [{charCode: 192, char: '6'}]},
        {bounds: [[517, 71, 65, 63]], states: [{charCode: 192, char: '7'}], key_state: KBD_UPPERCASE_DEFAULT},
        {bounds: [[589, 71, 65, 63]], states: [{charCode: 192, char: '8'}]},
        {bounds: [[661, 71, 65, 63]], states: [{charCode: 192, char: '9'}]},
        {bounds: [[733, 71, 65, 63]], states: [{charCode: 192, char: '0'}]},
        {bounds: [[806, 71, 65, 63]], states: [{charCode: 192, char: '-'}]},
        {bounds: [[880, 71, 65, 63]], states: [{charCode: 192, char: '='}]},
        {bounds: [[953, 71, 65, 63]], states: [{charCode: 192, char: '+'}]},
        {bounds: [[1024, 71, 139, 63]], event: 'delete'},

        // enter key has 2 bounds and one state
        {bounds: [[11, 143, 98, 71], [11, 213, 121, 62]], event: 'enter'},

        {bounds: [[118, 142, 64, 63]], states: [{charCode: 192, char: 'q'}, {charCode: 192, char: 'Q'}]},
        {bounds: [[190, 142, 64, 63]], states: [{charCode: 192, char: 'w'}, {charCode: 192, char: 'W'}]},
        {bounds: [[262, 142, 64, 63]], states: [{charCode: 192, char: 'e'}, {charCode: 192, char: 'E'}]},
        {bounds: [[334, 142, 64, 63]], states: [{charCode: 192, char: 'r'}, {charCode: 192, char: 'R'}]},
        {bounds: [[407, 142, 64, 63]], states: [{charCode: 192, char: 't'}, {charCode: 192, char: 'T'}]},
        {bounds: [[479, 142, 64, 63]], states: [{charCode: 192, char: 'y'}, {charCode: 192, char: 'Y'}]},
        {bounds: [[551, 142, 65, 63]], states: [{charCode: 192, char: 'u'}, {charCode: 192, char: 'U'}]},
        {bounds: [[623, 142, 65, 63]], states: [{charCode: 192, char: 'i'}, {charCode: 192, char: 'I'}]},
        {bounds: [[695, 142, 65, 63]], states: [{charCode: 192, char: 'o'}, {charCode: 192, char: 'O'}]},
        {bounds: [[768, 142, 64, 63]], states: [{charCode: 192, char: 'p'}, {charCode: 192, char: 'P'}]},
        {bounds: [[840, 142, 64, 63]], states: [{charCode: 192, char: '['}]},
        {bounds: [[912, 142, 65, 63]], states: [{charCode: 192, char: ']'}]},
        {bounds: [[984, 142, 65, 63]], states: [{charCode: 192, char: '\\'}]},
        {bounds: [[1055, 142, 65, 63]], states: [{charCode: 192, char: '|'}]},
        
        {bounds: [[1126, 143, 35, 72], [1008, 214, 153, 62]], event: 'enter'},

        {bounds: [[140, 213, 65, 63]], states: [{charCode: 192, char: 'a'}, {charCode: 192, char: 'A'}]},
        {bounds: [[211, 213, 64, 63]], states: [{charCode: 192, char: 's'}, {charCode: 192, char: 'S'}]},
        {bounds: [[283, 213, 65, 63]], states: [{charCode: 192, char: 'd'}, {charCode: 192, char: 'D'}]},
        {bounds: [[355, 213, 65, 63]], states: [{charCode: 192, char: 'f'}, {charCode: 192, char: 'F'}]},
        {bounds: [[428, 213, 64, 63]], states: [{charCode: 192, char: 'g'}, {charCode: 192, char: 'G'}]},
        {bounds: [[500, 213, 64, 63]], states: [{charCode: 192, char: 'h'}, {charCode: 192, char: 'H'}]},
        {bounds: [[572, 213, 65, 63]], states: [{charCode: 192, char: 'j'}, {charCode: 192, char: 'J'}]},
        {bounds: [[644, 213, 65, 63]], states: [{charCode: 192, char: 'k'}, {charCode: 192, char: 'K'}]},
        {bounds: [[716, 213, 65, 63]], states: [{charCode: 192, char: 'l'}, {charCode: 192, char: 'L'}]},
        {bounds: [[789, 213, 64, 63]], states: [{charCode: 192, char: ';'}]},
        {bounds: [[861, 213, 64, 63]], states: [{charCode: 192, char: '\''}]},
        {bounds: [[934, 213, 65, 63]], states: [{charCode: 192, char: ':'}]},

        {bounds: [[12, 283, 157, 63]], event: 'shift'},

        {bounds: [[176, 283, 65, 63]], states: [{charCode: 192, char: 'z'}, {charCode: 192, char: 'Z'}]},
        {bounds: [[249, 283, 64, 63]], states: [{charCode: 192, char: 'x'}, {charCode: 192, char: 'X'}]},
        {bounds: [[321, 283, 64, 63]], states: [{charCode: 192, char: 'c'}, {charCode: 192, char: 'C'}]},
        {bounds: [[393, 283, 64, 63]], states: [{charCode: 192, char: 'v'}, {charCode: 192, char: 'V'}]},
        {bounds: [[465, 283, 65, 63]], states: [{charCode: 192, char: 'b'}, {charCode: 192, char: 'B'}]},
        {bounds: [[537, 283, 65, 63]], states: [{charCode: 192, char: 'n'}, {charCode: 192, char: 'N'}]},
        {bounds: [[610, 283, 64, 63]], states: [{charCode: 192, char: 'm'}, {charCode: 192, char: 'M'}]},
        {bounds: [[682, 283, 64, 63]], states: [{charCode: 192, char: ','}]},
        {bounds: [[754, 283, 65, 63]], states: [{charCode: 192, char: '.'}]},
        {bounds: [[826, 283, 65, 63]], states: [{charCode: 192, char: '/'}]},
        {bounds: [[899, 283, 64, 63]], states: [{charCode: 192, char: '?'}], key_state: KBD_UPPERCASE_DEFAULT},

        {bounds: [[972, 283, 190, 63]], event: 'shift'},

        {bounds: [[249, 355, 573, 67]], states: [{charCode: 192, char: ' '}]},

        {bounds: [[899, 355, 263, 67]], event: 'submit'}

        

    ];

    this.keys_in_waitingline = [];
    this.keys_being_downloaded = [];

    for (var i = 0; i < keyProperties.length; i++) {
        //this.keys_in_waitingline.push(keyProperties[i]);
         //this.keys.push(new KeyboardKey(this, keyProperties[i]));
    }
    var tthis = this;
    this.keyboardtextureloaded = function() {
        if (Overlays.isLoaded(tthis.background)) {
            Script.clearInterval(tthis.keyboardtextureloaded_timer);
            for (var i = 0; i < keyProperties.length; i++) {
                tthis.keys.push(new KeyboardKey(tthis, keyProperties[i]));
            }
        }
    };
    this.keyboardtextureloaded_timer = Script.setInterval(this.keyboardtextureloaded, 250);
}

var keyboard = new Keyboard();
keyboard.onKeyPress = function() {
    print("Key press event test");
};

function scriptEnding() {
    keyboard.remove();
}
function mousePressEvent(event) {
    //var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
	//if (clickedOverlay != 0) {
	//	Overlays.deleteOverlay(clickedOverlay);
	//}
}

function onUpdate() {
    MyAvatar.getHeadFinalYaw();
    MyAvatar.getHeadFinalPitch();
}

Script.update.connect(onUpdate);
Script.scriptEnding.connect(scriptEnding);