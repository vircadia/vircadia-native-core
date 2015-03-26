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

    var WINDOW_WIDTH_2D = 160,
        WINDOW_MARGIN_2D = 12,
        WINDOW_FONT_2D = { size: 12 },
        WINDOW_FOREGROUND_COLOR_2D = { red: 240, green: 240, blue: 240 },
        WINDOW_FOREGROUND_ALPHA_2D = 0.9,
        WINDOW_HEADING_COLOR_2D = { red: 180, green: 180, blue: 180 },
        WINDOW_HEADING_ALPHA_2D = 0.9,
        WINDOW_BACKGROUND_COLOR_2D = { red: 80, green: 80, blue: 80 },
        WINDOW_BACKGROUND_ALPHA_2D = 0.7,
        windowPane2D,
        windowHeading2D,
        SCROLLBAR_BACKGROUND_WIDTH_2D = 12,
        SCROLLBAR_BACKGROUND_COLOR_2D = { red: 80, green: 80, blue: 80 },
        SCROLLBAR_BACKGROUND_ALPHA_2D = 0.8,
        scrollbarBackground2D,
        SCROLLBAR_BAR_MIN_HEIGHT = 5,
        SCROLLBAR_BAR_COLOR_2D = { red: 180, green: 180, blue: 180 },
        SCROLLBAR_BAR_ALPHA_2D = 0.8,
        scrollbarBar2D,
        scrollbarBackgroundHeight,
        scrollbarBarHeight,
        VISIBILITY_SPACER_2D = 12,                          // Space between list of users and visibility controls
        visibilityHeading2D,
        VISIBILITY_RADIO_SPACE = 16,
        visibilityControls2D,
        windowHeight,
        windowTextHeight,
        windowLineSpacing,
        windowLineHeight,                                   // = windowTextHeight + windowLineSpacing

        usersOnline,                                        // Raw users data
        linesOfUsers = [],                                  // Array of indexes pointing into usersOnline
        numUsersToDisplay = 0,
        firstUserToDisplay = 0,

        API_URL = "https://metaverse.highfidelity.com/api/v1/users?status=online",
        HTTP_GET_TIMEOUT = 60000,                           // ms = 1 minute
        usersRequest,
        processUsers,
        pollUsersTimedOut,
        usersTimer = null,
        USERS_UPDATE_TIMEOUT = 5000,                        // ms = 5s

        myVisibility,
        VISIBILITY_VALUES = ["all", "friends", "none"],
        visibilityInterval,
        VISIBILITY_POLL_INTERVAL = 5000,                    // ms = 5s

        MENU_NAME = "Tools",
        MENU_ITEM = "Users Online",
        MENU_ITEM_AFTER = "Chat...",

        isVisible = true,

        viewportHeight,
        isMirrorDisplay = false,
        isFullscreenMirror = false,

        isUsingScrollbars = false,
        isMovingScrollbar = false,
        scrollbarBackgroundPosition = {},
        scrollbarBarPosition = {},
        scrollbarBarClickedAt,                              // 0.0 .. 1.0
        scrollbarValue = 0.0,                               // 0.0 .. 1.0

        HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/",
        RADIO_BUTTON_SVG = HIFI_PUBLIC_BUCKET + "images/radio-button.svg",
        RADIO_BUTTON_SVG_DIAMETER = 14,
        RADIO_BUTTON_DISPLAY_SCALE = 0.7,                   // 1.0 = windowTextHeight
        radioButtonDiameter;

    function calculateWindowHeight() {
        var AUDIO_METER_HEIGHT = 52,
            MIRROR_HEIGHT = 220,
            nonUsersHeight,
            maxWindowHeight;

        // Reserve 5 lines for window heading plus visibility heading and controls
        // Subtract windowLineSpacing for both end of user list and end of controls
        nonUsersHeight = 5 * windowLineHeight - 2 * windowLineSpacing + VISIBILITY_SPACER_2D + 2 * WINDOW_MARGIN_2D;

        // Limit window to height of viewport minus VU meter and mirror if displayed
        windowHeight = linesOfUsers.length * windowLineHeight + nonUsersHeight;
        maxWindowHeight = viewportHeight - AUDIO_METER_HEIGHT;
        if (isMirrorDisplay && !isFullscreenMirror) {
            maxWindowHeight -= MIRROR_HEIGHT;
        }
        windowHeight = Math.max(Math.min(windowHeight, maxWindowHeight), nonUsersHeight);

        // Corresponding number of users to actually display
        numUsersToDisplay = Math.max(Math.round((windowHeight - nonUsersHeight) / windowLineHeight), 0);
        isUsingScrollbars = 0 < numUsersToDisplay && numUsersToDisplay < linesOfUsers.length;
        if (isUsingScrollbars) {
            firstUserToDisplay = Math.floor(scrollbarValue * (linesOfUsers.length - numUsersToDisplay));
        } else {
            firstUserToDisplay = 0;
            scrollbarValue = 0.0;
        }
    }

    function updateOverlayPositions() {
        var i,
            y;

        Overlays.editOverlay(windowPane2D, {
            y: viewportHeight - windowHeight
        });
        Overlays.editOverlay(windowHeading2D, {
            y: viewportHeight - windowHeight + WINDOW_MARGIN_2D
        });

        scrollbarBackgroundPosition.y = viewportHeight - windowHeight + WINDOW_MARGIN_2D + windowTextHeight;
        Overlays.editOverlay(scrollbarBackground2D, {
            y: scrollbarBackgroundPosition.y
        });
        scrollbarBarPosition.y = scrollbarBackgroundPosition.y + 1
                + scrollbarValue * (scrollbarBackgroundHeight - scrollbarBarHeight - 2);
        Overlays.editOverlay(scrollbarBar2D, {
            y: scrollbarBarPosition.y
        });
        Overlays.editOverlay(visibilityHeading2D, {
            y: viewportHeight - 4 * windowLineHeight + windowLineSpacing - WINDOW_MARGIN_2D
        });
        for (i = 0; i < visibilityControls2D.length; i += 1) {
            y = viewportHeight - (3 - i) * windowLineHeight + windowLineSpacing - WINDOW_MARGIN_2D;
            Overlays.editOverlay(visibilityControls2D[i].radioOverlay, { y: y });
            Overlays.editOverlay(visibilityControls2D[i].textOverlay, { y: y });
        }
    }

    function updateVisibilityControls() {
        var i,
            color;

        for (i = 0; i < visibilityControls2D.length; i += 1) {
            color = visibilityControls2D[i].selected ? WINDOW_FOREGROUND_COLOR_2D : WINDOW_HEADING_COLOR_2D;
            Overlays.editOverlay(visibilityControls2D[i].radioOverlay, {
                color: color,
                subImage: { y: visibilityControls2D[i].selected ? 0 : RADIO_BUTTON_SVG_DIAMETER }
            });
            Overlays.editOverlay(visibilityControls2D[i].textOverlay, { color: color });
        }
    }

    function updateUsersDisplay() {
        var displayText = "",
            user,
            userText,
            textWidth,
            maxTextWidth,
            ellipsisWidth,
            reducedTextWidth,
            i;

        maxTextWidth = WINDOW_WIDTH_2D - (isUsingScrollbars ? SCROLLBAR_BACKGROUND_WIDTH_2D : 0) - 2 * WINDOW_MARGIN_2D;
        ellipsisWidth = Overlays.textSize(windowPane2D, "...").width;
        reducedTextWidth = maxTextWidth - ellipsisWidth;

        for (i = 0; i < numUsersToDisplay; i += 1) {
            user = usersOnline[linesOfUsers[firstUserToDisplay + i]];
            userText = user.text;
            textWidth = user.textWidth;

            if (textWidth > maxTextWidth) {
                // Trim and append "..." to fit window width
                maxTextWidth = maxTextWidth - Overlays.textSize(windowPane2D, "...").width;
                while (textWidth > reducedTextWidth) {
                    userText = userText.slice(0, -1);
                    textWidth = Overlays.textSize(windowPane2D, userText).width;
                }
                userText += "...";
            }

            displayText += "\n" + userText;
        }

        displayText = displayText.slice(1);  // Remove leading "\n".

        Overlays.editOverlay(windowPane2D, {
            height: windowHeight,
            text: displayText
        });

        Overlays.editOverlay(windowHeading2D, {
            text: linesOfUsers.length > 0 ? "Users online" : "No users online"
        });

        scrollbarBackgroundHeight = numUsersToDisplay * windowLineHeight - windowLineSpacing / 2;
        Overlays.editOverlay(scrollbarBackground2D, {
            height: scrollbarBackgroundHeight,
            visible: isUsingScrollbars
        });
        scrollbarBarHeight = Math.max(numUsersToDisplay / linesOfUsers.length * scrollbarBackgroundHeight,
            SCROLLBAR_BAR_MIN_HEIGHT);
        Overlays.editOverlay(scrollbarBar2D, {
            height: scrollbarBarHeight,
            visible: isUsingScrollbars
        });

        updateOverlayPositions();
    }

    function pollUsers() {
        usersRequest = new XMLHttpRequest();
        usersRequest.open("GET", API_URL, true);
        usersRequest.timeout = HTTP_GET_TIMEOUT;
        usersRequest.ontimeout = pollUsersTimedOut;
        usersRequest.onreadystatechange = processUsers;
        usersRequest.send();
    }

    processUsers = function () {
        var response,
            myUsername,
            user,
            userText,
            i;

        if (usersRequest.readyState === usersRequest.DONE) {
            if (usersRequest.status === 200) {
                response = JSON.parse(usersRequest.responseText);
                if (response.status !== "success") {
                    print("Error: Request for users status returned status = " + response.status);
                    usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
                    return;
                }
                if (!response.hasOwnProperty("data") || !response.data.hasOwnProperty("users")) {
                    print("Error: Request for users status returned invalid data");
                    usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
                    return;
                }

                usersOnline = response.data.users;
                myUsername = GlobalServices.username;
                linesOfUsers = [];
                for (i = 0; i < usersOnline.length; i += 1) {
                    user = usersOnline[i];
                    if (user.username !== myUsername && user.online) {
                        userText = user.username;
                        if (user.location.root) {
                            userText += " @ " + user.location.root.name;
                        }

                        usersOnline[i].text = userText;
                        usersOnline[i].textWidth = Overlays.textSize(windowPane2D, userText).width;

                        linesOfUsers.push(i);
                    }
                }

                calculateWindowHeight();
                updateUsersDisplay();

            } else {
                print("Error: Request for users status returned " + usersRequest.status + " " + usersRequest.statusText);
                usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
                return;
            }

            usersTimer = Script.setTimeout(pollUsers, USERS_UPDATE_TIMEOUT);  // Update after finished processing.
        }
    };

    pollUsersTimedOut = function () {
        print("Error: Request for users status timed out");
        usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT);  // Try again after a longer delay.
    };

    function pollVisibility() {
        var currentVisibility = myVisibility;

        myVisibility = GlobalServices.findableBy;
        if (myVisibility !== currentVisibility) {
            updateVisibilityControls();
        }
    }

    function setVisible(visible) {
        var i;

        isVisible = visible;

        if (isVisible) {
            if (usersTimer === null) {
                pollUsers();
            }
        } else {
            Script.clearTimeout(usersTimer);
            usersTimer = null;
        }

        Overlays.editOverlay(windowPane2D, { visible: isVisible });
        Overlays.editOverlay(windowHeading2D, { visible: isVisible });
        Overlays.editOverlay(scrollbarBackground2D, { visible: isVisible && isUsingScrollbars });
        Overlays.editOverlay(scrollbarBar2D, { visible: isVisible && isUsingScrollbars });
        Overlays.editOverlay(visibilityHeading2D, { visible: isVisible });
        for (i = 0; i < visibilityControls2D.length; i += 1) {
            Overlays.editOverlay(visibilityControls2D[i].radioOverlay, { visible: isVisible });
            Overlays.editOverlay(visibilityControls2D[i].textOverlay, { visible: isVisible });
        }
    }

    function onMenuItemEvent(event) {
        if (event === MENU_ITEM) {
            setVisible(Menu.isOptionChecked(MENU_ITEM));
        }
    }

    function onMousePressEvent(event) {
        var clickedOverlay,
            numLinesBefore,
            overlayX,
            overlayY,
            minY,
            maxY,
            lineClicked,
            userClicked,
            i,
            visibilityChanged,
            delta;

        if (!isVisible) {
            return;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (clickedOverlay === windowPane2D) {

            overlayX = event.x - WINDOW_MARGIN_2D;
            overlayY = event.y - viewportHeight + windowHeight - WINDOW_MARGIN_2D - windowLineHeight;

            numLinesBefore = Math.round(overlayY / windowLineHeight);
            minY = numLinesBefore * windowLineHeight;
            maxY = minY + windowTextHeight;

            lineClicked = -1;
            if (minY <= overlayY && overlayY <= maxY) {
                lineClicked = numLinesBefore;
            }

            userClicked = firstUserToDisplay + lineClicked;

            if (0 <= userClicked && userClicked < linesOfUsers.length
                    && 0 <= overlayX && overlayX <= usersOnline[linesOfUsers[userClicked]].textWidth) {
                //print("Go to " + usersOnline[linesOfUsers[userClicked]].username);
                location.goToUser(usersOnline[linesOfUsers[userClicked]].username);
            }
        }

        visibilityChanged = false;
        for (i = 0; i < visibilityControls2D.length; i += 1) {
            // Don't need to test radioOverlay if it us under textOverlay.
            if (clickedOverlay === visibilityControls2D[i].textOverlay && event.x <= visibilityControls2D[i].optionWidth) {
                GlobalServices.findableBy = VISIBILITY_VALUES[i];
                visibilityChanged = true;
            }
        }
        if (visibilityChanged) {
            for (i = 0; i < visibilityControls2D.length; i += 1) {
                // Don't need to handle radioOverlay if it us under textOverlay.
                visibilityControls2D[i].selected = clickedOverlay === visibilityControls2D[i].textOverlay;
            }
            updateVisibilityControls();
        }

        if (clickedOverlay === scrollbarBar2D) {
            scrollbarBarClickedAt = (event.y - scrollbarBarPosition.y) / scrollbarBarHeight;
            isMovingScrollbar = true;
        }

        if (clickedOverlay === scrollbarBackground2D) {
            delta = scrollbarBarHeight / (scrollbarBackgroundHeight - scrollbarBarHeight);

            if (event.y < scrollbarBarPosition.y) {
                scrollbarValue = Math.max(scrollbarValue - delta, 0.0);
            } else {
                scrollbarValue = Math.min(scrollbarValue + delta, 1.0);
            }

            firstUserToDisplay = Math.floor(scrollbarValue * (linesOfUsers.length - numUsersToDisplay));
            updateOverlayPositions();
            updateUsersDisplay();
        }
    }

    function onMouseMoveEvent(event) {
        if (isMovingScrollbar) {
            if (scrollbarBackgroundPosition.x - WINDOW_MARGIN_2D <= event.x
                    && event.x <= scrollbarBackgroundPosition.x + SCROLLBAR_BACKGROUND_WIDTH_2D + WINDOW_MARGIN_2D
                    && scrollbarBackgroundPosition.y - WINDOW_MARGIN_2D <= event.y
                    && event.y <= scrollbarBackgroundPosition.y + scrollbarBackgroundHeight + WINDOW_MARGIN_2D) {
                scrollbarValue = (event.y - scrollbarBarClickedAt * scrollbarBarHeight - scrollbarBackgroundPosition.y)
                    / (scrollbarBackgroundHeight - scrollbarBarHeight - 2);
                scrollbarValue = Math.min(Math.max(scrollbarValue, 0.0), 1.0);
                firstUserToDisplay = Math.floor(scrollbarValue * (linesOfUsers.length - numUsersToDisplay));
                updateOverlayPositions();
                updateUsersDisplay();
            } else {
                isMovingScrollbar = false;
            }
        }
    }

    function onMouseReleaseEvent() {
        isMovingScrollbar = false;
    }

    function onScriptUpdate() {
        var oldViewportHeight = viewportHeight,
            oldIsMirrorDisplay = isMirrorDisplay,
            oldIsFullscreenMirror = isFullscreenMirror,
            MIRROR_MENU_ITEM = "Mirror",
            FULLSCREEN_MIRROR_MENU_ITEM = "Fullscreen Mirror";

        viewportHeight = Controller.getViewportDimensions().y;
        isMirrorDisplay = Menu.isOptionChecked(MIRROR_MENU_ITEM);
        isFullscreenMirror = Menu.isOptionChecked(FULLSCREEN_MIRROR_MENU_ITEM);

        if (viewportHeight !== oldViewportHeight
                || isMirrorDisplay !== oldIsMirrorDisplay
                || isFullscreenMirror !== oldIsFullscreenMirror) {
            calculateWindowHeight();
            updateUsersDisplay();
            updateOverlayPositions();
        }
    }

    function setUp() {
        var textSizeOverlay,
            optionText;

        textSizeOverlay = Overlays.addOverlay("text", { font: WINDOW_FONT_2D, visible: false });
        windowTextHeight = Math.floor(Overlays.textSize(textSizeOverlay, "1").height);
        windowLineSpacing = Math.floor(Overlays.textSize(textSizeOverlay, "1\n2").height - 2 * windowTextHeight);
        windowLineHeight = windowTextHeight + windowLineSpacing;
        radioButtonDiameter = RADIO_BUTTON_DISPLAY_SCALE * windowTextHeight;
        Overlays.deleteOverlay(textSizeOverlay);

        viewportHeight = Controller.getViewportDimensions().y;

        calculateWindowHeight();

        windowPane2D = Overlays.addOverlay("text", {
            x: 0,
            y: viewportHeight,  // Start up off-screen
            width: WINDOW_WIDTH_2D,
            height: windowHeight,
            topMargin: WINDOW_MARGIN_2D + windowLineHeight,
            leftMargin: WINDOW_MARGIN_2D,
            color: WINDOW_FOREGROUND_COLOR_2D,
            alpha: WINDOW_FOREGROUND_ALPHA_2D,
            backgroundColor: WINDOW_BACKGROUND_COLOR_2D,
            backgroundAlpha: WINDOW_BACKGROUND_ALPHA_2D,
            text: "",
            font: WINDOW_FONT_2D,
            visible: isVisible
        });

        windowHeading2D = Overlays.addOverlay("text", {
            x: WINDOW_MARGIN_2D,
            y: viewportHeight,
            width: WINDOW_WIDTH_2D - 2 * WINDOW_MARGIN_2D,
            height: windowTextHeight,
            topMargin: 0,
            leftMargin: 0,
            color: WINDOW_HEADING_COLOR_2D,
            alpha: WINDOW_HEADING_ALPHA_2D,
            backgroundAlpha: 0.0,
            text: "No users online",
            font: WINDOW_FONT_2D,
            visible: isVisible
        });

        scrollbarBackgroundPosition = {
            x: WINDOW_WIDTH_2D - 0.5 * WINDOW_MARGIN_2D - SCROLLBAR_BACKGROUND_WIDTH_2D,
            y: viewportHeight
        };
        scrollbarBackground2D = Overlays.addOverlay("text", {
            x: scrollbarBackgroundPosition.x,
            y: scrollbarBackgroundPosition.y,
            width: SCROLLBAR_BACKGROUND_WIDTH_2D,
            height: windowTextHeight,
            backgroundColor: SCROLLBAR_BACKGROUND_COLOR_2D,
            backgroundAlpha: SCROLLBAR_BACKGROUND_ALPHA_2D,
            text: "",
            visible: isVisible && isUsingScrollbars
        });

        scrollbarBarPosition = {
            x: WINDOW_WIDTH_2D - 0.5 * WINDOW_MARGIN_2D - SCROLLBAR_BACKGROUND_WIDTH_2D + 1,
            y: viewportHeight
        };
        scrollbarBar2D = Overlays.addOverlay("text", {
            x: scrollbarBarPosition.x,
            y: scrollbarBarPosition.y,
            width: SCROLLBAR_BACKGROUND_WIDTH_2D - 2,
            height: windowTextHeight,
            backgroundColor: SCROLLBAR_BAR_COLOR_2D,
            backgroundAlpha: SCROLLBAR_BAR_ALPHA_2D,
            text: "",
            visible: isVisible && isUsingScrollbars
        });

        visibilityHeading2D = Overlays.addOverlay("text", {
            x: WINDOW_MARGIN_2D,
            y: viewportHeight,
            width: WINDOW_WIDTH_2D - 2 * WINDOW_MARGIN_2D,
            height: windowTextHeight,
            topMargin: 0,
            leftMargin: 0,
            color: WINDOW_HEADING_COLOR_2D,
            alpha: WINDOW_HEADING_ALPHA_2D,
            backgroundAlpha: 0.0,
            text: "I am visible to:",
            font: WINDOW_FONT_2D,
            visible: isVisible
        });

        myVisibility = GlobalServices.findableBy;
        if (!/^(all|friends|none)$/.test(myVisibility)) {
            print("Error: Unrecognized findableBy value");
            myVisibility = "";
        }

        optionText = "everyone";
        visibilityControls2D = [{
            radioOverlay: Overlays.addOverlay("image", {  // Create first so that it is under textOverlay.
                x: WINDOW_MARGIN_2D,
                y: viewportHeight,
                width: radioButtonDiameter,
                height: radioButtonDiameter,
                imageURL: RADIO_BUTTON_SVG,
                subImage: {
                    x: 0,
                    y: RADIO_BUTTON_SVG_DIAMETER,  // Off
                    width: RADIO_BUTTON_SVG_DIAMETER,
                    height: RADIO_BUTTON_SVG_DIAMETER
                },
                color: WINDOW_HEADING_COLOR_2D,
                alpha: WINDOW_FOREGROUND_ALPHA_2D
            }),
            textOverlay: Overlays.addOverlay("text", {
                x: WINDOW_MARGIN_2D,
                y: viewportHeight,
                width: WINDOW_WIDTH_2D - SCROLLBAR_BACKGROUND_WIDTH_2D - 2 * WINDOW_MARGIN_2D,
                height: windowTextHeight,
                topMargin: 0,
                leftMargin: VISIBILITY_RADIO_SPACE,
                color: WINDOW_HEADING_COLOR_2D,
                alpha: WINDOW_FOREGROUND_ALPHA_2D,
                backgroundAlpha: 0.0,
                text: optionText,
                font: WINDOW_FONT_2D,
                visible: isVisible
            }),
            selected: myVisibility === VISIBILITY_VALUES[0]
        }];
        visibilityControls2D[0].optionWidth = WINDOW_MARGIN_2D + VISIBILITY_RADIO_SPACE
            + Overlays.textSize(visibilityControls2D[0].textOverlay, optionText).width;

        optionText = "my friends";
        visibilityControls2D[1] = {
            radioOverlay: Overlays.cloneOverlay(visibilityControls2D[0].radioOverlay),
            textOverlay: Overlays.cloneOverlay(visibilityControls2D[0].textOverlay),
            selected: myVisibility === VISIBILITY_VALUES[1]
        };
        Overlays.editOverlay(visibilityControls2D[1].textOverlay, { text: optionText });
        visibilityControls2D[1].optionWidth = WINDOW_MARGIN_2D + VISIBILITY_RADIO_SPACE
            + Overlays.textSize(visibilityControls2D[1].textOverlay, optionText).width;

        optionText = "no one";
        visibilityControls2D[2] = {
            radioOverlay: Overlays.cloneOverlay(visibilityControls2D[0].radioOverlay),
            textOverlay: Overlays.cloneOverlay(visibilityControls2D[0].textOverlay),
            selected: myVisibility === VISIBILITY_VALUES[2]
        };
        Overlays.editOverlay(visibilityControls2D[2].textOverlay, { text: optionText });
        visibilityControls2D[2].optionWidth = WINDOW_MARGIN_2D + VISIBILITY_RADIO_SPACE
            + Overlays.textSize(visibilityControls2D[2].textOverlay, optionText).width;

        updateVisibilityControls();

        Controller.mousePressEvent.connect(onMousePressEvent);
        Controller.mouseMoveEvent.connect(onMouseMoveEvent);
        Controller.mouseReleaseEvent.connect(onMouseReleaseEvent);

        Menu.addMenuItem({
            menuName: MENU_NAME,
            menuItemName: MENU_ITEM,
            afterItem: MENU_ITEM_AFTER,
            isCheckable: true,
            isChecked: isVisible
        });
        Menu.menuItemEvent.connect(onMenuItemEvent);

        Script.update.connect(onScriptUpdate);

        visibilityInterval = Script.setInterval(pollVisibility, VISIBILITY_POLL_INTERVAL);

        pollUsers();

    }

    function tearDown() {
        var i;

        Menu.removeMenuItem(MENU_NAME, MENU_ITEM);

        Script.clearInterval(visibilityInterval);
        Script.clearTimeout(usersTimer);
        Overlays.deleteOverlay(windowPane2D);
        Overlays.deleteOverlay(windowHeading2D);
        Overlays.deleteOverlay(scrollbarBackground2D);
        Overlays.deleteOverlay(scrollbarBar2D);
        Overlays.deleteOverlay(visibilityHeading2D);
        for (i = 0; i <= visibilityControls2D.length; i += 1) {
            Overlays.deleteOverlay(visibilityControls2D[i].textOverlay);
            Overlays.deleteOverlay(visibilityControls2D[i].radioOverlay);
        }
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
