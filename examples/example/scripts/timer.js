//
//  timer.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var one_timer = Script.setTimeout(function() { print("One time timer fired!"); }, 10000);
var multiple_timer = Script.setInterval(function() { print("Repeating timer fired!"); }, 1000);

// this would stop a scheduled single shot timer
Script.clearTimeout(one_timer);

// this stops the repeating timer
Script.clearInterval(multiple_timer);