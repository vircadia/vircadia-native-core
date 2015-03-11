//
//  users.js
//  examples
//
//  Created by David Rowe on 9 Mar 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var usersWindow = (function () {

    var WINDOW_WIDTH_2D = 150,
        WINDOW_MARGIN_2D = 12,
        WINDOW_FOREGROUND_COLOR_2D = { red: 240, green: 240, blue: 240 },
        WINDOW_FOREGROUND_ALPHA_2D = 0.9,
        WINDOW_HEADING_COLOR_2D = { red: 180, green: 180, blue: 180 },
        WINDOW_HEADING_ALPHA_2D = 0.9,
        WINDOW_BACKGROUND_COLOR_2D = { red: 80, green: 80, blue: 80 },
        WINDOW_BACKGROUND_ALPHA_2D = 0.7,
        windowHeight = 0,
        usersPane2D,
        usersHeading2D,
        USERS_FONT_2D = { size: 14 },
        usersLineHeight,
        usersLineSpacing,

        usersOnline,    // Raw data
        linesOfUsers,   // Array of indexes pointing into usersOnline

        API_URL = "https://metaverse.highfidelity.io/api/v1/users?status=online",
        HTTP_GET_TIMEOUT = 60000,  // ms = 1 minute
        usersRequest,
        processUsers,
        usersTimedOut,
        usersTimer = null,
        UPDATE_TIMEOUT = 5000,  // ms = 5s

        MENU_NAME = "Tools",
        MENU_ITEM = "Users Online",
        MENU_ITEM_AFTER = "Chat...",

        isVisible = true,

        viewportHeight;

    function onMousePressEvent(event) {
        var clickedOverlay,
            numLinesBefore,
            overlayX,
            overlayY,
            minY,
            maxY,
            lineClicked;

        if (!isVisible) {
            return;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
        if (clickedOverlay === usersPane2D) {

            overlayX = event.x - WINDOW_MARGIN_2D;
            overlayY = event.y - viewportHeight + windowHeight - WINDOW_MARGIN_2D - (usersLineHeight + usersLineSpacing);

            numLinesBefore = Math.floor(overlayY / (usersLineHeight + usersLineSpacing));
            minY = numLinesBefore * (usersLineHeight + usersLineSpacing);
            maxY = minY + usersLineHeight;

            lineClicked = -1;
            if (minY <= overlayY && overlayY <= maxY) {
                lineClicked = numLinesBefore;
            }

            if (0 <= lineClicked && lineClicked < linesOfUsers.length
                    && overlayX <= usersOnline[linesOfUsers[lineClicked]].usernameWidth) {
                //print("Go to " + usersOnline[linesOfUsers[lineClicked]].username);
                location.goToUser(usersOnline[linesOfUsers[lineClicked]].username);
            }
        }
    }

    function updateWindow() {
        var displayText = "",
            myUsername,
            user,
            i;

        myUsername = GlobalServices.username;
        linesOfUsers = [];
        for (i = 0; i < usersOnline.length; i += 1) {
            user = usersOnline[i];
            if (user.username !== myUsername && user.online) {
                usersOnline[i].usernameWidth = Overlays.textSize(usersPane2D, user.username).width;
                linesOfUsers.push(i);
                displayText += "\n" + user.username;
            }
        }

        windowHeight = (linesOfUsers.length > 0 ? linesOfUsers.length + 1 : 1) * (usersLineHeight + usersLineSpacing)
                - usersLineSpacing + 2 * WINDOW_MARGIN_2D;  // First or only line is for heading

        Overlays.editOverlay(usersPane2D, {
            y: viewportHeight - windowHeight,
            height: windowHeight,
            text: displayText
        });

        Overlays.editOverlay(usersHeading2D, {
            y: viewportHeight - windowHeight + WINDOW_MARGIN_2D,
            text: linesOfUsers.length > 0 ? "Online" : "No users online"
        });
    }

    function requestUsers() {
        usersRequest = new XMLHttpRequest();
        usersRequest.open("GET", API_URL, true);
        usersRequest.timeout = HTTP_GET_TIMEOUT;
        usersRequest.ontimeout = usersTimedOut;
        usersRequest.onreadystatechange = processUsers;
        usersRequest.send();
    }

    processUsers = function () {
        var response;

        if (usersRequest.readyState === usersRequest.DONE) {
            if (usersRequest.status === 200) {
                response = JSON.parse(usersRequest.responseText);
                if (response.status !== "success") {
                    print("Error: Request for users status returned status = " + response.status);
                    usersTimer = Script.setTimeout(requestUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
                    return;
                }
                if (!response.hasOwnProperty("data") || !response.data.hasOwnProperty("users")) {
                    print("Error: Request for users status returned invalid data");
                    usersTimer = Script.setTimeout(requestUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
                    return;
                }

                usersOnline = response.data.users;
                updateWindow();
            } else {
                print("Error: Request for users status returned " + usersRequest.status + " " + usersRequest.statusText);
                usersTimer = Script.setTimeout(requestUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
                return;
            }

            usersTimer = Script.setTimeout(requestUsers, UPDATE_TIMEOUT);  // Update after finished processing.
        }
    };

    usersTimedOut = function () {
        print("Error: Request for users status timed out");
        usersTimer = Script.setTimeout(requestUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
    };

    function setVisible(visible) {
        isVisible = visible;

        if (isVisible) {
            if (usersTimer === null) {
                requestUsers();
            }
        } else {
            Script.clearTimeout(usersTimer);
            usersTimer = null;
        }

        Overlays.editOverlay(usersPane2D, { visible: isVisible });
        Overlays.editOverlay(usersHeading2D, { visible: isVisible });
    }

    function onMenuItemEvent(event) {
        if (event === MENU_ITEM) {
            setVisible(Menu.isOptionChecked(MENU_ITEM));
        }
    }

    function update() {
        viewportHeight = Controller.getViewportDimensions().y;
        Overlays.editOverlay(usersPane2D, {
            y: viewportHeight - windowHeight
        });
        Overlays.editOverlay(usersHeading2D, {
            y: viewportHeight - windowHeight + WINDOW_MARGIN_2D
        });
    }

    function setUp() {
        usersPane2D = Overlays.addOverlay("text", {
            bounds: { x: 0, y: 0, width: WINDOW_WIDTH_2D, height: 0 },
            topMargin: WINDOW_MARGIN_2D,
            leftMargin: WINDOW_MARGIN_2D,
            color: WINDOW_FOREGROUND_COLOR_2D,
            alpha: WINDOW_FOREGROUND_ALPHA_2D,
            backgroundColor: WINDOW_BACKGROUND_COLOR_2D,
            backgroundAlpha: WINDOW_BACKGROUND_ALPHA_2D,
            text: "",
            font: USERS_FONT_2D,
            visible: isVisible
        });

        usersHeading2D = Overlays.addOverlay("text", {
            x: WINDOW_MARGIN_2D,
            width: WINDOW_WIDTH_2D - 2 * WINDOW_MARGIN_2D,
            height: USERS_FONT_2D.size,
            topMargin: 0,
            leftMargin: 0,
            color: WINDOW_HEADING_COLOR_2D,
            alpha: WINDOW_HEADING_ALPHA_2D,
            backgroundAlpha: 0.0,
            text: "No users online",
            font: USERS_FONT_2D,
            visible: isVisible
        });

        viewportHeight = Controller.getViewportDimensions().y;

        usersLineHeight = Math.floor(Overlays.textSize(usersPane2D, "1").height);
        usersLineSpacing = Math.floor(Overlays.textSize(usersPane2D, "1\n2").height - 2 * usersLineHeight);

        Controller.mousePressEvent.connect(onMousePressEvent);

        Menu.addMenuItem({
            menuName: MENU_NAME,
            menuItemName: MENU_ITEM,
            afterItem: MENU_ITEM_AFTER,
            isCheckable: true,
            isChecked: isVisible
        });
        Menu.menuItemEvent.connect(onMenuItemEvent);

        Script.update.connect(update);

        requestUsers();
    }

    function tearDown() {
        Menu.removeMenuItem(MENU_NAME, MENU_ITEM);

        Script.clearTimeout(usersTimer);
        Overlays.deleteOverlay(usersHeading2D);
        Overlays.deleteOverlay(usersPane2D);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
