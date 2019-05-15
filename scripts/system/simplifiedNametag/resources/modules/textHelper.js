//
//  Simplified Nametag
//  textHelper.js
//  Created by Milad Nazeri on 2019-03-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Module to help calculate text size
//


// *************************************
// START MAPS
// *************************************
// #region MAPS


var charMap = {
    a: 0.05,
    b: 0.051, 
    c: 0.05,
    d: 0.051,
    e: 0.05,
    f: 0.035,
    g: 0.051,
    h: 0.051,
    i: 0.025,
    j: 0.025,
    k: 0.05,
    l: 0.025,
    m: 0.0775,
    n: 0.051,
    o: 0.051,
    p: 0.051,
    q: 0.051,
    r: 0.035,
    s: 0.05,
    t: 0.035,
    u: 0.051,
    v: 0.05,
    w: 0.07,
    x: 0.05,
    y: 0.05,
    z: 0.05,
    A: 0.06,
    B: 0.06,
    C: 0.06,
    D: 0.06,
    E: 0.05,
    F: 0.05,
    G: 0.06,
    H: 0.0625,
    I: 0.0275,
    J: 0.05,
    K: 0.06,
    L: 0.05,
    M: 0.075,
    N: 0.0625,
    O: 0.0625,
    P: 0.06,
    Q: 0.0625,
    R: 0.06,
    S: 0.06,
    T: 0.06,
    U: 0.06,
    V: 0.06,
    W: 0.075,
    X: 0.06,
    Y: 0.06,
    Z: 0.06
};

var symbolMap = {
    "!": 0.025,
    "@": 0.08,
    "#": 0.07,
    "$": 0.058,
    "%": 0.07,
    "^": 0.04,
    "&": 0.06,
    "*": 0.04,
    "(": 0.04,
    ")": 0.04,
    "_": 0.041,
    "{": 0.034,
    "}": 0.034,
    "/": 0.04,
    "|": 0.02,
    "<": 0.049,
    ">": 0.049,
    "[": 0.0300,
    "]": 0.0300,
    ".": 0.0260,
    ",": 0.0260,
    "?": 0.048,
    "~": 0.0610,
    "`": 0.0310,
    "+": 0.0510,
    "=": 0.0510
};


// #endregion
// *************************************
// END MAPS
// *************************************

var _this = null;
function TextHelper(){
    _this = this; 

    this.text = "";
    this.textArray = "";
    this.lineHeight = 0;
    this.totalTextLength = 0;
    this.scaler = 1.0;
}


// *************************************
// START UTILITY
// *************************************
// #region UTILITY


// Split the string into a text array to be operated on
function createTextArray(){
    _this.textArray = _this.text.split("");
}


// Account for the text length
function adjustForScale(defaultTextLength){
    _this.totalTextLength = defaultTextLength * _this.scaler;
}


// #endregion
// *************************************
// END UTILITY
// *************************************

// #endregion
// *************************************
// END name
// *************************************

// *************************************
// START API
// *************************************
// #region API


// Set the text that needs to be calculated on
function setText(newText){
    _this.text = newText;
    createTextArray();

    return _this;
}


// Set the line height which helps calculate the font size
var DEFAULT_LINE_HEIGHT = 0.1;
function setLineHeight(newLineHeight){
    _this.lineHeight = newLineHeight;
    _this.scaler = _this.lineHeight / DEFAULT_LINE_HEIGHT;

    return _this;
}

    
// Calculate the sign dimensions
var DEFAULT_CHAR_SIZE = 0.025;
var NUMBER = 0.05;
var DIGIT_REGEX = /\d/g;
var WHITE_SPACE_REGEX = /[ ]/g;
var SPACE = 0.018;
function getTotalTextLength(){
    // Map the string array to it's sizes
    var lengthArray = _this.textArray.map(function(letter){
        if (charMap[letter]){
            return charMap[letter];
        } else if (letter.match(DIGIT_REGEX)){
            return NUMBER;
        } else if (symbolMap[letter]) {
            return symbolMap[letter];
        } else if (letter.match(WHITE_SPACE_REGEX)) {
            return SPACE;
        } else {
            return DEFAULT_CHAR_SIZE;
        }
    });

    // add up all the values in the array
    var defaultTextLength = lengthArray.reduce(function(prev, curr){
        return prev + curr;
    }, 0);

    adjustForScale(defaultTextLength);

    return _this.totalTextLength;
}


// #endregion
// *************************************
// END API
// *************************************

TextHelper.prototype = {
    setText: setText,
    setLineHeight: setLineHeight,
    getTotalTextLength: getTotalTextLength
};

module.exports = TextHelper;

// var text = new TextHelper();
// text.setText("lowbar");
// text.setLineHeight("0.1");
// text.getTotalTextLength();