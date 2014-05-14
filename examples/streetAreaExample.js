//
//  streetAreaExample.js
//  examples
//
//  Created by Ryan Huffman on 5/4/14
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script showing how to load JSON data using XMLHttpRequest.
//
//  URL Macro created by Thijs Wenker.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var url = "https://script.google.com/macros/s/AKfycbwIo4lmF-qUwX1Z-9eA_P-g2gse9oFhNcjVyyksGukyDDEFXgU/exec?action=listOwners&domain=alpha.highfidelity.io";
print("Loading street data from " + url);

var req = new XMLHttpRequest();

// Set response type to "json".  This will tell XMLHttpRequest to parse the response data as json, so req.response can be used
// as a regular javascript object
req.responseType = 'json';

req.open("GET", url, false);
req.send();

if (req.status == 200) {
    for (var domain in req.response) {
        print("DOMAIN: " + domain);
        var locations = req.response[domain];
        var userAreas = [];
        for (var i = 0; i < locations.length; i++) {
            var loc = locations[i];
            var x1 = loc[1],
                x2 = loc[2],
                y1 = loc[3],
                y2 = loc[4];
            userAreas.push({
                username: loc[0],
                area: Math.abs(x2 - x1) * Math.abs(y2 - y1),
            });
        }
        userAreas.sort(function(a, b) { return a.area > b.area ? -1 : (a.area < b.area ? 1 : 0) });
        for (var i = 0; i < userAreas.length; i++) {
            print(userAreas[i].username + ": " + userAreas[i].area + " sq units");
        }
    }
} else {
    print("Error loading data: " + req.status + " " + req.statusText + ", " + req.errorCode);
}
