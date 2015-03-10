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

    var WINDOW_BOUNDS_2D = { x: 100, y: 100, width: 300, height: 0 },
        WINDOW_MARGIN_2D = 12,
        WINDOW_FOREGROUND_COLOR_2D = { red: 240, green: 240, blue: 240 },
        WINDOW_FOREGROUND_ALPHA_2D = 0.9,
        WINDOW_BACKGROUND_COLOR_2D = { red: 120, green: 120, blue: 120 },
        WINDOW_BACKGROUND_ALPHA_2D = 0.7,
        usersPane2D,
        USERS_PANE_TEXT_WIDTH_2D = WINDOW_BOUNDS_2D.width - 2 * WINDOW_MARGIN_2D,
        USERS_FONT_2D = { size: 14 },
        usersLineHeight,
        usersLineSpacing,
        USERNAME_SPACER = "\u00a0\u00a0\u00a0\u00a0",  // Nonbreaking spaces
        usernameSpacerWidth2D,

        usersOnline,
        linesOfUserIndexes = [],  // 2D array of lines of indexes into usersOnline

        API_URL = "https://metaverse.highfidelity.io/api/v1/users?status=online",
        HTTP_GET_TIMEOUT = 60000,  // ms = 1 minute
        usersRequest,
        processUsers,
        usersTimedOut,
        usersTimer = null,
        UPDATE_TIMEOUT = 5000;  // ms = 5s

        MENU_NAME = "Tools",
        MENU_ITEM = "Users Online",
        MENI_ITEM_AFTER = "Chat...",

        isVisible = false;

    function onMousePressEvent(event) {
        var clickedOverlay,
            numLinesBefore,
            overlayX,
            overlayY,
            minY,
            maxY,
            lineClicked,
            i,
            userIndex,
            foundUser;

        if (!isVisible) {
            return;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });
        if (clickedOverlay === usersPane2D) {
            overlayX = event.x - WINDOW_BOUNDS_2D.x - WINDOW_MARGIN_2D;
            overlayY = event.y - WINDOW_BOUNDS_2D.y - WINDOW_MARGIN_2D;

            numLinesBefore = Math.floor(overlayY / (usersLineHeight + usersLineSpacing));
            minY = numLinesBefore * (usersLineHeight + usersLineSpacing);
            maxY = minY + usersLineHeight;

            lineClicked = -1;
            if (minY <= overlayY && overlayY <= maxY) {
                lineClicked = numLinesBefore;
            }

            if (0 <= lineClicked && lineClicked < linesOfUserIndexes.length) {
                foundUser = false;
                i = 0;
                while (!foundUser && i < linesOfUserIndexes[lineClicked].length) {
                    userIndex = linesOfUserIndexes[lineClicked][i];
                    foundUser = usersOnline[userIndex].leftX <= overlayX && overlayX <= usersOnline[userIndex].rightX;
                    i += 1;
                }

                if (foundUser) {
                    location.goToUser(usersOnline[userIndex].username);
                }
            }
        }
    }

    function updateWindow() {
        var displayText = "",
            numUsersDisplayed = 0,
            lineText = "",
            lineWidth = 0,
            myUsername = GlobalServices.username,
            usernameWidth,
            user,
            i;

        // Create 2D array of lines x IDs into usersOnline.
        // Augment usersOnline entries with x-coordinates of left and right of displayed username.
        linesOfUserIndexes = [];
        for (i = 0; i < usersOnline.length; i += 1) {
            user = usersOnline[i];
            if (user.username !== myUsername && user.online) {

                usernameWidth = Overlays.textSize(usersPane2D, user.username).width;

                if (linesOfUserIndexes.length === 0
                        || lineWidth + usernameSpacerWidth2D + usernameWidth > USERS_PANE_TEXT_WIDTH_2D) {

                    displayText += "\n" + lineText;

                    // New line
                    linesOfUserIndexes.push([i]);
                    usersOnline[i].leftX = 0;
                    usersOnline[i].rightX = usernameWidth;

                    lineText = user.username;
                    lineWidth = usernameWidth;

                } else {
                    // Append
                    linesOfUserIndexes[linesOfUserIndexes.length - 1].push(i);
                    usersOnline[i].leftX = lineWidth + usernameSpacerWidth2D;
                    usersOnline[i].rightX = lineWidth + usernameSpacerWidth2D + usernameWidth;

                    lineText += USERNAME_SPACER + user.username;
                    lineWidth = usersOnline[i].rightX;
                }

                numUsersDisplayed += 1;
            }
        }

        displayText += "\n" + lineText;
        displayText = displayText.slice(2);  // Remove leading "\n"s.

        Overlays.editOverlay(usersPane2D, {
            text: numUsersDisplayed > 0 ? displayText : "No users online",
            height: (numUsersDisplayed > 0 ? linesOfUserIndexes.length : 1) * (usersLineHeight + usersLineSpacing)
                - usersLineSpacing + 2 * WINDOW_MARGIN_2D
        });
    }

    function requestUsers() {
        usersRequest = new XMLHttpRequest();
        usersRequest.open("GET", API_URL, true);
        usersRequest.timeout = HTTP_GET_TIMEOUT * 1000;
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
    }

    function onMenuItemEvent(event) {
        if (event === MENU_ITEM) {
            setVisible(Menu.isOptionChecked(MENU_ITEM));
        }
    }

    function setUp() {
        usersPane2D = Overlays.addOverlay("text", {
            bounds: WINDOW_BOUNDS_2D,
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

        usersLineHeight = Math.floor(Overlays.textSize(usersPane2D, "1").height);
        usersLineSpacing = Math.floor(Overlays.textSize(usersPane2D, "1\n2").height - 2 * usersLineHeight);

        usernameSpacerWidth2D = Overlays.textSize(usersPane2D, USERNAME_SPACER).width;

        Controller.mousePressEvent.connect(onMousePressEvent);

        Menu.addMenuItem({
            menuName: MENU_NAME,
            menuItemName: MENU_ITEM,
            afterItem: MENI_ITEM_AFTER,
            isCheckable: true,
            isChecked: isVisible
        });
        Menu.menuItemEvent.connect(onMenuItemEvent);

        requestUsers();
    }

    function tearDown() {
        Menu.removeMenuItem(MENU_NAME, MENU_ITEM);

        Script.clearTimeout(usersTimer);
        Overlays.deleteOverlay(usersPane2D);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
