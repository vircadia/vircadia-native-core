//
//  currentAPI.js
//  examples
//
//  Created by Clément Brisset on 5/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var array = [];
function listKeys(string, object) {
    if (string === "listKeys" || string === "array" || string === "buffer" || string === "i") {
        return;
    }
    
    if (typeof(object) !== "object" || object === null) {
        array.push(string + " " + typeof(object));
        return;
    }
    
    var keys = Object.keys(object);
    for (var i = 0; i < keys.length; ++i) {
        if (string === "") {
            listKeys(keys[i], object[keys[i]]);
        } else if (keys[i] !== "parent") {
            listKeys(string + "." + keys[i], object[keys[i]]);
        }
    }
}

listKeys("", this);
array.sort();

var buffer = "\n======= JS API list =======";
for (var i = 0; i < array.length; ++i) {
    buffer += "\n" + array[i];
}
buffer += "\n========= API END =========\n";

print(buffer);
