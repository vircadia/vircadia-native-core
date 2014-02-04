var one_timer = Script.setTimeout(function() { print("One time timer fired!"); }, 10000);
var multiple_timer = Script.setInterval(function() { print("Repeating timer fired!"); }, 1000);

// this would stop a scheduled single shot timer
Script.clearTimeout(one_timer);

// this stops the repeating timer
Script.clearInterval(multiple_timer);