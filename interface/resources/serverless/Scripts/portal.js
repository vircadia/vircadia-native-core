//
//  portal.js
//  Created by Zach Fox on 2019-05-23
//  Copyright 2019 High Fidelity, Inc.
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function () {
    var Portal = function() {};


    Portal.prototype = {
        enterEntity: function (id) {
            var properties = Entities.getEntityProperties(id, ["userData"]);
            var userData;

            try {
                userData = JSON.parse(properties.userData);
            } catch (e) {
                console.error("Error parsing userData: ", e);
            }

            if (userData) {
                if (userData.destination) {
                    var destination = userData.destination;

                    if (userData.destination.indexOf("bookmark:") > -1) {
                        var bookmarkName = userData.destination.replace("bookmark:", "");
                        destination = LocationBookmarks.getAddress(bookmarkName);
                    }
                    
                    Window.location = destination;
                } else {
                    console.log("Please specify `destination` inside this entity's `userData`!");
                    return;
                }
            } else {
                console.log("Please specify this entity's `userData`! See README.md for instructions.");
                return;
            }
        }
    };

    
    return new Portal();
});
