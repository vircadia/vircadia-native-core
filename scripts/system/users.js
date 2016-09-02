"use strict";

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

(function() { // BEGIN LOCAL_SCOPE

// resolve these paths immediately
var MIN_MAX_BUTTON_SVG = Script.resolvePath("assets/images/tools/min-max-toggle.svg");
var BASE_URL = Script.resolvePath("assets/images/tools/");

var PopUpMenu = function (properties) {
    var value = properties.value,
        promptOverlay,
        valueOverlay,
        buttonOverlay,
        optionOverlays = [],
        isDisplayingOptions = false,
        OPTION_MARGIN = 4,

        MIN_MAX_BUTTON_SVG_WIDTH = 17.1,
        MIN_MAX_BUTTON_SVG_HEIGHT = 32.5,
        MIN_MAX_BUTTON_WIDTH = 14,
        MIN_MAX_BUTTON_HEIGHT = MIN_MAX_BUTTON_WIDTH;

    function positionDisplayOptions() {
        var y,
            i;

        y = properties.y - (properties.values.length - 1) * properties.lineHeight - OPTION_MARGIN;

        for (i = 0; i < properties.values.length; i += 1) {
            Overlays.editOverlay(optionOverlays[i], {
                y: y
            });
            y += properties.lineHeight;
        }
    }

    function showDisplayOptions() {
        var i,
            yOffScreen = Controller.getViewportDimensions().y;

        for (i = 0; i < properties.values.length; i += 1) {
            optionOverlays[i] = Overlays.addOverlay("text", {
                x: properties.x + properties.promptWidth,
                y: yOffScreen,
                width: properties.width - properties.promptWidth,
                height: properties.textHeight + OPTION_MARGIN, // Only need to add margin at top to balance descenders
                topMargin: OPTION_MARGIN,
                leftMargin: OPTION_MARGIN,
                color: properties.optionColor,
                alpha: properties.optionAlpha,
                backgroundColor: properties.popupBackgroundColor,
                backgroundAlpha: properties.popupBackgroundAlpha,
                text: properties.displayValues[i],
                font: properties.font,
                visible: true
            });
        }

        positionDisplayOptions();

        isDisplayingOptions = true;
    }

    function deleteDisplayOptions() {
        var i;

        for (i = 0; i < optionOverlays.length; i += 1) {
            Overlays.deleteOverlay(optionOverlays[i]);
        }

        isDisplayingOptions = false;
    }

    function handleClick(overlay) {
        var clicked = false,
            i;

        if (overlay === valueOverlay || overlay === buttonOverlay) {
            showDisplayOptions();
            return true;
        }

        if (isDisplayingOptions) {
            for (i = 0; i < optionOverlays.length; i += 1) {
                if (overlay === optionOverlays[i]) {
                    value = properties.values[i];
                    Overlays.editOverlay(valueOverlay, {
                        text: properties.displayValues[i]
                    });
                    clicked = true;
                }
            }

            deleteDisplayOptions();
        }

        return clicked;
    }

    function updatePosition(x, y) {
        properties.x = x;
        properties.y = y;
        Overlays.editOverlay(promptOverlay, {
            x: x,
            y: y
        });
        Overlays.editOverlay(valueOverlay, {
            x: x + properties.promptWidth,
            y: y - OPTION_MARGIN
        });
        Overlays.editOverlay(buttonOverlay, {
            x: x + properties.width - MIN_MAX_BUTTON_WIDTH - 1,
            y: y - OPTION_MARGIN + 1
        });
        if (isDisplayingOptions) {
            positionDisplayOptions();
        }
    }

    function setVisible(visible) {
        Overlays.editOverlay(promptOverlay, {
            visible: visible
        });
        Overlays.editOverlay(valueOverlay, {
            visible: visible
        });
        Overlays.editOverlay(buttonOverlay, {
            visible: visible
        });
    }

    function tearDown() {
        Overlays.deleteOverlay(promptOverlay);
        Overlays.deleteOverlay(valueOverlay);
        Overlays.deleteOverlay(buttonOverlay);
        if (isDisplayingOptions) {
            deleteDisplayOptions();
        }
    }

    function getValue() {
        return value;
    }

    function setValue(newValue) {
        var index;

        index = properties.values.indexOf(newValue);
        if (index !== -1) {
            value = newValue;
            Overlays.editOverlay(valueOverlay, {
                text: properties.displayValues[index]
            });
        }
    }

    promptOverlay = Overlays.addOverlay("text", {
        x: properties.x,
        y: properties.y,
        width: properties.promptWidth,
        height: properties.textHeight,
        topMargin: 0,
        leftMargin: 0,
        color: properties.promptColor,
        alpha: properties.promptAlpha,
        backgroundColor: properties.promptBackgroundColor,
        backgroundAlpha: properties.promptBackgroundAlpha,
        text: properties.prompt,
        font: properties.font,
        visible: properties.visible
    });

    valueOverlay = Overlays.addOverlay("text", {
        x: properties.x + properties.promptWidth,
        y: properties.y,
        width: properties.width - properties.promptWidth,
        height: properties.textHeight + OPTION_MARGIN, // Only need to add margin at top to balance descenders
        topMargin: OPTION_MARGIN,
        leftMargin: OPTION_MARGIN,
        color: properties.optionColor,
        alpha: properties.optionAlpha,
        backgroundColor: properties.optionBackgroundColor,
        backgroundAlpha: properties.optionBackgroundAlpha,
        text: properties.displayValues[properties.values.indexOf(value)],
        font: properties.font,
        visible: properties.visible
    });

    buttonOverlay = Overlays.addOverlay("image", {
        x: properties.x + properties.width - MIN_MAX_BUTTON_WIDTH - 1,
        y: properties.y,
        width: MIN_MAX_BUTTON_WIDTH,
        height: MIN_MAX_BUTTON_HEIGHT,
        imageURL: MIN_MAX_BUTTON_SVG,
        subImage: {
            x: 0,
            y: 0,
            width: MIN_MAX_BUTTON_SVG_WIDTH,
            height: MIN_MAX_BUTTON_SVG_HEIGHT / 2
        },
        //color: properties.buttonColor,
        alpha: properties.buttonAlpha,
        visible: properties.visible
    });

    return {
        updatePosition: updatePosition,
        setVisible: setVisible,
        handleClick: handleClick,
        tearDown: tearDown,
        getValue: getValue,
        setValue: setValue
    };
};

var usersWindow = (function () {

    var WINDOW_WIDTH = 260,
        WINDOW_MARGIN = 12,
        WINDOW_BASE_MARGIN = 6, // A little less is needed in order look correct
        WINDOW_FONT = {
            size: 12
        },
        WINDOW_FOREGROUND_COLOR = {
            red: 240,
            green: 240,
            blue: 240
        },
        WINDOW_FOREGROUND_ALPHA = 0.95,
        WINDOW_HEADING_COLOR = {
            red: 180,
            green: 180,
            blue: 180
        },
        WINDOW_HEADING_ALPHA = 0.95,
        WINDOW_BACKGROUND_COLOR = {
            red: 80,
            green: 80,
            blue: 80
        },
        WINDOW_BACKGROUND_ALPHA = 0.8,
        windowPane,
        windowHeading,

        // Window border is similar to that of edit.js.
        WINDOW_BORDER_WIDTH = WINDOW_WIDTH + 2 * WINDOW_BASE_MARGIN,
        WINDOW_BORDER_TOP_MARGIN = 2 * WINDOW_BASE_MARGIN,
        WINDOW_BORDER_BOTTOM_MARGIN = WINDOW_BASE_MARGIN,
        WINDOW_BORDER_LEFT_MARGIN = WINDOW_BASE_MARGIN,
        WINDOW_BORDER_RADIUS = 4,
        WINDOW_BORDER_COLOR = { red: 255, green: 255, blue: 255 },
        WINDOW_BORDER_ALPHA = 0.5,
        windowBorder,

        MIN_MAX_BUTTON_SVG = BASE_URL + "min-max-toggle.svg",
        MIN_MAX_BUTTON_SVG_WIDTH = 17.1,
        MIN_MAX_BUTTON_SVG_HEIGHT = 32.5,
        MIN_MAX_BUTTON_WIDTH = 14,
        MIN_MAX_BUTTON_HEIGHT = MIN_MAX_BUTTON_WIDTH,
        MIN_MAX_BUTTON_COLOR = {
            red: 255,
            green: 255,
            blue: 255
        },
        MIN_MAX_BUTTON_ALPHA = 0.9,
        minimizeButton,
        SCROLLBAR_BACKGROUND_WIDTH = 12,
        SCROLLBAR_BACKGROUND_COLOR = {
            red: 70,
            green: 70,
            blue: 70
        },
        SCROLLBAR_BACKGROUND_ALPHA = 0.8,
        scrollbarBackground,
        SCROLLBAR_BAR_MIN_HEIGHT = 5,
        SCROLLBAR_BAR_COLOR = {
            red: 170,
            green: 170,
            blue: 170
        },
        SCROLLBAR_BAR_ALPHA = 0.8,
        SCROLLBAR_BAR_SELECTED_ALPHA = 0.95,
        scrollbarBar,
        scrollbarBackgroundHeight,
        scrollbarBarHeight,
        FRIENDS_BUTTON_SPACER = 6, // Space before add/remove friends button
        FRIENDS_BUTTON_SVG = BASE_URL + "add-remove-friends.svg",
        FRIENDS_BUTTON_SVG_WIDTH = 107,
        FRIENDS_BUTTON_SVG_HEIGHT = 27,
        FRIENDS_BUTTON_WIDTH = FRIENDS_BUTTON_SVG_WIDTH,
        FRIENDS_BUTTON_HEIGHT = FRIENDS_BUTTON_SVG_HEIGHT,
        FRIENDS_BUTTON_COLOR = {
            red: 225,
            green: 225,
            blue: 225
        },
        FRIENDS_BUTTON_ALPHA = 0.95,
        FRIENDS_WINDOW_URL = "https://metaverse.highfidelity.com/user/friends",
        FRIENDS_WINDOW_WIDTH = 290,
        FRIENDS_WINDOW_HEIGHT = 500,
        FRIENDS_WINDOW_TITLE = "Add/Remove Friends",
        friendsButton,
        friendsWindow,

        OPTION_BACKGROUND_COLOR = {
            red: 60,
            green: 60,
            blue: 60
        },
        OPTION_BACKGROUND_ALPHA = 0.1,

        DISPLAY_SPACER = 12, // Space before display control
        DISPLAY_PROMPT = "Show me:",
        DISPLAY_PROMPT_WIDTH = 60,
        DISPLAY_EVERYONE = "everyone",
        DISPLAY_FRIENDS = "friends",
        DISPLAY_VALUES = [DISPLAY_EVERYONE, DISPLAY_FRIENDS],
        DISPLAY_DISPLAY_VALUES = DISPLAY_VALUES,
        DISPLAY_OPTIONS_BACKGROUND_COLOR = {
            red: 120,
            green: 120,
            blue: 120
        },
        DISPLAY_OPTIONS_BACKGROUND_ALPHA = 0.9,
        displayControl,

        VISIBILITY_SPACER = 6, // Space before visibility control
        VISIBILITY_PROMPT = "Visible to:",
        VISIBILITY_PROMPT_WIDTH = 60,
        VISIBILITY_ALL = "all",
        VISIBILITY_FRIENDS = "friends",
        VISIBILITY_NONE = "none",
        VISIBILITY_VALUES = [VISIBILITY_ALL, VISIBILITY_FRIENDS, VISIBILITY_NONE],
        VISIBILITY_DISPLAY_VALUES = ["everyone", "friends", "no one"],
        visibilityControl,

        windowHeight,
        windowBorderHeight,
        windowTextHeight,
        windowLineSpacing,
        windowLineHeight, // = windowTextHeight + windowLineSpacing
        windowMinimumHeight,

        usersOnline, // Raw users data
        linesOfUsers = [], // Array of indexes pointing into usersOnline
        numUsersToDisplay = 0,
        firstUserToDisplay = 0,

        API_URL = "https://metaverse.highfidelity.com/api/v1/users?status=online",
        API_FRIENDS_FILTER = "&filter=friends",
        HTTP_GET_TIMEOUT = 60000, // ms = 1 minute
        usersRequest,
        processUsers,
        pollUsersTimedOut,
        usersTimer = null,
        USERS_UPDATE_TIMEOUT = 5000, // ms = 5s

        showMe,
        myVisibility,

        MENU_NAME = "View",
        MENU_ITEM = "Users Online",
        MENU_ITEM_AFTER = "Overlays",

        SETTING_USERS_SHOW_ME = "UsersWindow.ShowMe",
        SETTING_USERS_VISIBLE_TO = "UsersWindow.VisibleTo",
        SETTING_USERS_WINDOW_MINIMIZED = "UsersWindow.Minimized",
        SETTING_USERS_WINDOW_OFFSET = "UsersWindow.Offset",
        // +ve x, y values are offset from left, top of screen; -ve from right, bottom.

        isLoggedIn = false,
        isVisible = true,
        isMinimized = true,
        isBorderVisible = false,

        viewport,
        isMirrorDisplay = false,
        isFullscreenMirror = false,

        windowPosition = {},  // Bottom left corner of window pane.
        isMovingWindow = false,
        movingClickOffset = { x: 0, y: 0 },

        isUsingScrollbars = false,
        isMovingScrollbar = false,
        scrollbarBackgroundPosition = {},
        scrollbarBarPosition = {},
        scrollbarBarClickedAt, // 0.0 .. 1.0
        scrollbarValue = 0.0; // 0.0 .. 1.0

    function isValueTrue(value) {
        // Work around Boolean Settings values being read as string when Interface starts up but as Booleans when re-read after
        // Being written if refresh script.
        return value === true || value === "true";
    }

    function calculateWindowHeight() {
        var AUDIO_METER_HEIGHT = 52,
            MIRROR_HEIGHT = 220,
            nonUsersHeight,
            maxWindowHeight;

        if (isMinimized) {
            windowHeight = windowTextHeight + WINDOW_MARGIN + WINDOW_BASE_MARGIN;
            windowBorderHeight = windowHeight + WINDOW_BORDER_TOP_MARGIN + WINDOW_BORDER_BOTTOM_MARGIN;
            return;
        }

        // Reserve space for title, friends button, and option controls
        nonUsersHeight = WINDOW_MARGIN + windowLineHeight + FRIENDS_BUTTON_SPACER + FRIENDS_BUTTON_HEIGHT + DISPLAY_SPACER
            + windowLineHeight + VISIBILITY_SPACER
            + windowLineHeight + WINDOW_BASE_MARGIN;

        // Limit window to height of viewport above window position minus VU meter and mirror if displayed
        windowHeight = linesOfUsers.length * windowLineHeight - windowLineSpacing + nonUsersHeight;
        maxWindowHeight = windowPosition.y - AUDIO_METER_HEIGHT;
        if (isMirrorDisplay && !isFullscreenMirror) {
            maxWindowHeight -= MIRROR_HEIGHT;
        }
        windowHeight = Math.max(Math.min(windowHeight, maxWindowHeight), nonUsersHeight);
        windowBorderHeight = windowHeight + WINDOW_BORDER_TOP_MARGIN + WINDOW_BORDER_BOTTOM_MARGIN;

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

    function saturateWindowPosition() {
        windowPosition.x = Math.max(0, Math.min(viewport.x - WINDOW_WIDTH, windowPosition.x));
        windowPosition.y = Math.max(windowMinimumHeight, Math.min(viewport.y, windowPosition.y));
    }

    function updateOverlayPositions() {
        // Overlay positions are all relative to windowPosition; windowPosition is the position of the windowPane overlay.
        var windowLeft = windowPosition.x,
            windowTop = windowPosition.y - windowHeight,
            x,
            y;

        Overlays.editOverlay(windowBorder, {
            x: windowPosition.x - WINDOW_BORDER_LEFT_MARGIN,
            y: windowTop - WINDOW_BORDER_TOP_MARGIN
        });
        Overlays.editOverlay(windowPane, {
            x: windowLeft,
            y: windowTop
        });
        Overlays.editOverlay(windowHeading, {
            x: windowLeft + WINDOW_MARGIN,
            y: windowTop + WINDOW_MARGIN
        });

        Overlays.editOverlay(minimizeButton, {
            x: windowLeft + WINDOW_WIDTH - WINDOW_MARGIN / 2 - MIN_MAX_BUTTON_WIDTH,
            y: windowTop + WINDOW_MARGIN / 2
        });

        scrollbarBackgroundPosition.x = windowLeft + WINDOW_WIDTH - 0.5 * WINDOW_MARGIN - SCROLLBAR_BACKGROUND_WIDTH;
        scrollbarBackgroundPosition.y = windowTop + WINDOW_MARGIN + windowTextHeight;
        Overlays.editOverlay(scrollbarBackground, {
            x: scrollbarBackgroundPosition.x,
            y: scrollbarBackgroundPosition.y
        });
        scrollbarBarPosition.y = scrollbarBackgroundPosition.y + 1
            + scrollbarValue * (scrollbarBackgroundHeight - scrollbarBarHeight - 2);
        Overlays.editOverlay(scrollbarBar, {
            x: scrollbarBackgroundPosition.x + 1,
            y: scrollbarBarPosition.y
        });

        x = windowLeft + WINDOW_MARGIN;
        y = windowPosition.y - FRIENDS_BUTTON_HEIGHT - DISPLAY_SPACER
            - windowLineHeight - VISIBILITY_SPACER
            - windowLineHeight - WINDOW_BASE_MARGIN;
        Overlays.editOverlay(friendsButton, {
            x: x,
            y: y
        });

        y += FRIENDS_BUTTON_HEIGHT + DISPLAY_SPACER;
        displayControl.updatePosition(x, y);

        y += windowLineHeight + VISIBILITY_SPACER;
        visibilityControl.updatePosition(x, y);
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

        if (!isMinimized) {
            maxTextWidth = WINDOW_WIDTH - (isUsingScrollbars ? SCROLLBAR_BACKGROUND_WIDTH : 0) - 2 * WINDOW_MARGIN;
            ellipsisWidth = Overlays.textSize(windowPane, "...").width;
            reducedTextWidth = maxTextWidth - ellipsisWidth;

            for (i = 0; i < numUsersToDisplay; i += 1) {
                user = usersOnline[linesOfUsers[firstUserToDisplay + i]];
                userText = user.text;
                textWidth = user.textWidth;

                if (textWidth > maxTextWidth) {
                    // Trim and append "..." to fit window width
                    maxTextWidth = maxTextWidth - Overlays.textSize(windowPane, "...").width;
                    while (textWidth > reducedTextWidth) {
                        userText = userText.slice(0, -1);
                        textWidth = Overlays.textSize(windowPane, userText).width;
                    }
                    userText += "...";
                }

                displayText += "\n" + userText;
            }

            displayText = displayText.slice(1); // Remove leading "\n".

            scrollbarBackgroundHeight = numUsersToDisplay * windowLineHeight - windowLineSpacing / 2;
            Overlays.editOverlay(scrollbarBackground, {
                height: scrollbarBackgroundHeight,
                visible: isLoggedIn && isUsingScrollbars
            });
            scrollbarBarHeight = Math.max(numUsersToDisplay / linesOfUsers.length * scrollbarBackgroundHeight,
                SCROLLBAR_BAR_MIN_HEIGHT);
            Overlays.editOverlay(scrollbarBar, {
                height: scrollbarBarHeight,
                visible: isLoggedIn && isUsingScrollbars
            });
        }

        Overlays.editOverlay(windowBorder, {
            height: windowBorderHeight
        });

        Overlays.editOverlay(windowPane, {
            height: windowHeight,
            text: displayText
        });

        Overlays.editOverlay(windowHeading, {
            text: linesOfUsers.length > 0 ? "Users online" : "No users online"
        });
    }

    function updateOverlayVisibility() {
        Overlays.editOverlay(windowBorder, {
            visible: isLoggedIn && isVisible && isBorderVisible
        });
        Overlays.editOverlay(windowPane, {
            visible: isLoggedIn && isVisible
        });
        Overlays.editOverlay(windowHeading, {
            visible: isLoggedIn && isVisible
        });
        Overlays.editOverlay(minimizeButton, {
            visible: isLoggedIn && isVisible
        });
        Overlays.editOverlay(scrollbarBackground, {
            visible: isLoggedIn && isVisible && isUsingScrollbars && !isMinimized
        });
        Overlays.editOverlay(scrollbarBar, {
            visible: isLoggedIn && isVisible && isUsingScrollbars && !isMinimized
        });
        Overlays.editOverlay(friendsButton, {
            visible: isLoggedIn && isVisible && !isMinimized
        });
        displayControl.setVisible(isLoggedIn && isVisible && !isMinimized);
        visibilityControl.setVisible(isLoggedIn && isVisible && !isMinimized);
    }

    function checkLoggedIn() {
        var wasLoggedIn = isLoggedIn;

        isLoggedIn = Account.isLoggedIn();
        if (isLoggedIn !== wasLoggedIn) {
            updateOverlayVisibility();
        }
    }

    function pollUsers() {
        var url = API_URL;

        if (showMe === DISPLAY_FRIENDS) {
            url += API_FRIENDS_FILTER;
        }

        usersRequest = new XMLHttpRequest();
        usersRequest.open("GET", url, true);
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
                    usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT); // Try again after a longer delay.
                    return;
                }
                if (!response.hasOwnProperty("data") || !response.data.hasOwnProperty("users")) {
                    print("Error: Request for users status returned invalid data");
                    usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT); // Try again after a longer delay.
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
                        usersOnline[i].textWidth = Overlays.textSize(windowPane, userText).width;

                        linesOfUsers.push(i);
                    }
                }

                calculateWindowHeight();
                updateUsersDisplay();
                updateOverlayPositions();

                checkLoggedIn();

            } else {
                print("Error: Request for users status returned " + usersRequest.status + " " + usersRequest.statusText);
                usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT); // Try again after a longer delay.
                return;
            }

            usersTimer = Script.setTimeout(pollUsers, USERS_UPDATE_TIMEOUT); // Update after finished processing.
        }
    };

    pollUsersTimedOut = function () {
        print("Error: Request for users status timed out");
        usersTimer = Script.setTimeout(pollUsers, HTTP_GET_TIMEOUT); // Try again after a longer delay.
    };

    function setVisible(visible) {
        isVisible = visible;

        if (isVisible) {
            if (usersTimer === null) {
                pollUsers();
            }
        } else {
            Script.clearTimeout(usersTimer);
            usersTimer = null;
        }

        updateOverlayVisibility();
    }

    function setMinimized(minimized) {
        isMinimized = minimized;
        Overlays.editOverlay(minimizeButton, {
            subImage: {
                y: isMinimized ? MIN_MAX_BUTTON_SVG_HEIGHT / 2 : 0
            }
        });
        updateOverlayVisibility();
        Settings.setValue(SETTING_USERS_WINDOW_MINIMIZED, isMinimized);
    }

    function onMenuItemEvent(event) {
        if (event === MENU_ITEM) {
            setVisible(Menu.isOptionChecked(MENU_ITEM));
        }
    }

    function onFindableByChanged(event) {
        if (VISIBILITY_VALUES.indexOf(event) !== -1) {
            myVisibility = event;
            visibilityControl.setValue(event);
            Settings.setValue(SETTING_USERS_VISIBLE_TO, myVisibility);
        } else {
            print("Error: Unrecognized onFindableByChanged value: " + event);
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
            delta;

        if (!isVisible) {
            return;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });

        if (displayControl.handleClick(clickedOverlay)) {
            if (usersTimer !== null) {
                Script.clearTimeout(usersTimer);
                usersTimer = null;
            }
            pollUsers();
            showMe = displayControl.getValue();
            Settings.setValue(SETTING_USERS_SHOW_ME, showMe);
            return;
        }

        if (visibilityControl.handleClick(clickedOverlay)) {
            myVisibility = visibilityControl.getValue();
            GlobalServices.findableBy = myVisibility;
            Settings.setValue(SETTING_USERS_VISIBLE_TO, myVisibility);
            return;
        }

        if (clickedOverlay === windowPane) {

            overlayX = event.x - windowPosition.x - WINDOW_MARGIN;
            overlayY = event.y - windowPosition.y + windowHeight - WINDOW_MARGIN - windowLineHeight;

            numLinesBefore = Math.round(overlayY / windowLineHeight);
            minY = numLinesBefore * windowLineHeight;
            maxY = minY + windowTextHeight;

            lineClicked = -1;
            if (minY <= overlayY && overlayY <= maxY) {
                lineClicked = numLinesBefore;
            }

            userClicked = firstUserToDisplay + lineClicked;

            if (0 <= userClicked && userClicked < linesOfUsers.length && 0 <= overlayX
                    && overlayX <= usersOnline[linesOfUsers[userClicked]].textWidth) {
                //print("Go to " + usersOnline[linesOfUsers[userClicked]].username);
                location.goToUser(usersOnline[linesOfUsers[userClicked]].username);
            }

            return;
        }

        if (clickedOverlay === minimizeButton) {
            setMinimized(!isMinimized);
            calculateWindowHeight();
            updateOverlayPositions();
            updateUsersDisplay();
            return;
        }

        if (clickedOverlay === scrollbarBar) {
            scrollbarBarClickedAt = (event.y - scrollbarBarPosition.y) / scrollbarBarHeight;
            Overlays.editOverlay(scrollbarBar, {
                backgroundAlpha: SCROLLBAR_BAR_SELECTED_ALPHA
            });
            isMovingScrollbar = true;
            return;
        }

        if (clickedOverlay === scrollbarBackground) {
            delta = scrollbarBarHeight / (scrollbarBackgroundHeight - scrollbarBarHeight);

            if (event.y < scrollbarBarPosition.y) {
                scrollbarValue = Math.max(scrollbarValue - delta, 0.0);
            } else {
                scrollbarValue = Math.min(scrollbarValue + delta, 1.0);
            }

            firstUserToDisplay = Math.floor(scrollbarValue * (linesOfUsers.length - numUsersToDisplay));
            updateOverlayPositions();
            updateUsersDisplay();
            return;
        }

        if (clickedOverlay === friendsButton) {
            if (!friendsWindow) {
                friendsWindow = new OverlayWebWindow({
                    title: FRIENDS_WINDOW_TITLE,
                    width: FRIENDS_WINDOW_WIDTH,
                    height: FRIENDS_WINDOW_HEIGHT,
                    visible: false
                });
            }
            friendsWindow.setURL(FRIENDS_WINDOW_URL);
            friendsWindow.setVisible(true);
            friendsWindow.raise();
            return;
        }

        if (clickedOverlay === windowBorder) {
            movingClickOffset = {
                x: event.x - windowPosition.x,
                y: event.y - windowPosition.y
            };

            isMovingWindow = true;
        }
    }

    function onMouseMoveEvent(event) {
        var isVisible;

        if (!isLoggedIn) {
            return;
        }

        if (isMovingScrollbar) {
            if (scrollbarBackgroundPosition.x - WINDOW_MARGIN <= event.x
                    && event.x <= scrollbarBackgroundPosition.x + SCROLLBAR_BACKGROUND_WIDTH + WINDOW_MARGIN
                    && scrollbarBackgroundPosition.y - WINDOW_MARGIN <= event.y
                    && event.y <= scrollbarBackgroundPosition.y + scrollbarBackgroundHeight + WINDOW_MARGIN) {
                scrollbarValue = (event.y - scrollbarBarClickedAt * scrollbarBarHeight - scrollbarBackgroundPosition.y)
                    / (scrollbarBackgroundHeight - scrollbarBarHeight - 2);
                scrollbarValue = Math.min(Math.max(scrollbarValue, 0.0), 1.0);
                firstUserToDisplay = Math.floor(scrollbarValue * (linesOfUsers.length - numUsersToDisplay));
                updateOverlayPositions();
                updateUsersDisplay();
            } else {
                Overlays.editOverlay(scrollbarBar, {
                    backgroundAlpha: SCROLLBAR_BAR_ALPHA
                });
                isMovingScrollbar = false;
            }
        }

        if (isMovingWindow) {
            windowPosition = {
                x: event.x - movingClickOffset.x,
                y: event.y - movingClickOffset.y
            };

            saturateWindowPosition();
            calculateWindowHeight();
            updateOverlayPositions();
            updateUsersDisplay();

        } else {

            isVisible = isBorderVisible;
            if (isVisible) {
                isVisible = windowPosition.x - WINDOW_BORDER_LEFT_MARGIN <= event.x
                    && event.x <= windowPosition.x - WINDOW_BORDER_LEFT_MARGIN + WINDOW_BORDER_WIDTH
                    && windowPosition.y - windowHeight - WINDOW_BORDER_TOP_MARGIN <= event.y
                    && event.y <= windowPosition.y + WINDOW_BORDER_BOTTOM_MARGIN;
            } else {
                isVisible = windowPosition.x <= event.x && event.x <= windowPosition.x + WINDOW_WIDTH
                    && windowPosition.y - windowHeight <= event.y && event.y <= windowPosition.y;
            }
            if (isVisible !== isBorderVisible) {
                isBorderVisible = isVisible;
                Overlays.editOverlay(windowBorder, {
                    visible: isBorderVisible
                });
            }
        }
    }

    function onMouseReleaseEvent() {
        var offset = {};

        if (isMovingScrollbar) {
            Overlays.editOverlay(scrollbarBar, {
                backgroundAlpha: SCROLLBAR_BAR_ALPHA
            });
            isMovingScrollbar = false;
        }

        if (isMovingWindow) {
            // Save offset of bottom of window to nearest edge of the window.
            offset.x = (windowPosition.x + WINDOW_WIDTH / 2 < viewport.x / 2)
                ? windowPosition.x : windowPosition.x - viewport.x;
            offset.y = (windowPosition.y < viewport.y / 2)
                ? windowPosition.y : windowPosition.y - viewport.y;
            Settings.setValue(SETTING_USERS_WINDOW_OFFSET, JSON.stringify(offset));
            isMovingWindow = false;
        }
    }

    function onScriptUpdate() {
        var oldViewport = viewport,
            oldIsMirrorDisplay = isMirrorDisplay,
            oldIsFullscreenMirror = isFullscreenMirror,
            MIRROR_MENU_ITEM = "Mirror",
            FULLSCREEN_MIRROR_MENU_ITEM = "Fullscreen Mirror";

        viewport = Controller.getViewportDimensions();
        isMirrorDisplay = Menu.isOptionChecked(MIRROR_MENU_ITEM);
        isFullscreenMirror = Menu.isOptionChecked(FULLSCREEN_MIRROR_MENU_ITEM);

        if (viewport.y !== oldViewport.y || isMirrorDisplay !== oldIsMirrorDisplay
                || isFullscreenMirror !== oldIsFullscreenMirror) {
            calculateWindowHeight();
            updateUsersDisplay();
        }

        if (viewport.y !== oldViewport.y) {
            if (windowPosition.y > oldViewport.y / 2) {
                // Maintain position w.r.t. bottom of window.
                windowPosition.y = viewport.y - (oldViewport.y - windowPosition.y);
            }
        }

        if (viewport.x !== oldViewport.x) {
            if (windowPosition.x + (WINDOW_WIDTH / 2) > oldViewport.x / 2) {
                // Maintain position w.r.t. right of window.
                windowPosition.x = viewport.x - (oldViewport.x - windowPosition.x);
            }
        }

        updateOverlayPositions();
    }

    function setUp() {
        var textSizeOverlay,
            offsetSetting,
            offset = {},
            hmdViewport;

        textSizeOverlay = Overlays.addOverlay("text", {
            font: WINDOW_FONT,
            visible: false
        });
        windowTextHeight = Math.floor(Overlays.textSize(textSizeOverlay, "1").height);
        windowLineSpacing = Math.floor(Overlays.textSize(textSizeOverlay, "1\n2").height - 2 * windowTextHeight);
        windowLineHeight = windowTextHeight + windowLineSpacing;
        windowMinimumHeight = windowTextHeight + WINDOW_MARGIN + WINDOW_BASE_MARGIN;
        Overlays.deleteOverlay(textSizeOverlay);

        viewport = Controller.getViewportDimensions();

        offsetSetting = Settings.getValue(SETTING_USERS_WINDOW_OFFSET);
        if (offsetSetting !== "") {
            offset = JSON.parse(Settings.getValue(SETTING_USERS_WINDOW_OFFSET));
        }
        if (offset.hasOwnProperty("x") && offset.hasOwnProperty("y")) {
            windowPosition.x = offset.x < 0 ? viewport.x + offset.x : offset.x;
            windowPosition.y = offset.y <= 0 ? viewport.y + offset.y : offset.y;
        } else {
            hmdViewport = Controller.getRecommendedOverlayRect();
            windowPosition = {
                x: (viewport.x - hmdViewport.width) / 2,  // HMD viewport is narrower than screen.
                y: hmdViewport.height  // HMD viewport starts at top of screen but only extends down so far.
            };
        }

        saturateWindowPosition();
        calculateWindowHeight();

        windowBorder = Overlays.addOverlay("rectangle", {
            x: 0,
            y: viewport.y,   // Start up off-screen
            width: WINDOW_BORDER_WIDTH,
            height: windowBorderHeight,
            radius: WINDOW_BORDER_RADIUS,
            color: WINDOW_BORDER_COLOR,
            alpha: WINDOW_BORDER_ALPHA,
            visible: false
        });

        windowPane = Overlays.addOverlay("text", {
            x: 0,
            y: viewport.y,
            width: WINDOW_WIDTH,
            height: windowHeight,
            topMargin: WINDOW_MARGIN + windowLineHeight,
            leftMargin: WINDOW_MARGIN,
            color: WINDOW_FOREGROUND_COLOR,
            alpha: WINDOW_FOREGROUND_ALPHA,
            backgroundColor: WINDOW_BACKGROUND_COLOR,
            backgroundAlpha: WINDOW_BACKGROUND_ALPHA,
            text: "",
            font: WINDOW_FONT,
            visible: false
        });

        windowHeading = Overlays.addOverlay("text", {
            x: 0,
            y: viewport.y,
            width: WINDOW_WIDTH - 2 * WINDOW_MARGIN,
            height: windowTextHeight,
            topMargin: 0,
            leftMargin: 0,
            color: WINDOW_HEADING_COLOR,
            alpha: WINDOW_HEADING_ALPHA,
            backgroundAlpha: 0.0,
            text: "No users online",
            font: WINDOW_FONT,
            visible: false
        });

        minimizeButton = Overlays.addOverlay("image", {
            x: 0,
            y: viewport.y,
            width: MIN_MAX_BUTTON_WIDTH,
            height: MIN_MAX_BUTTON_HEIGHT,
            imageURL: MIN_MAX_BUTTON_SVG,
            subImage: {
                x: 0,
                y: 0,
                width: MIN_MAX_BUTTON_SVG_WIDTH,
                height: MIN_MAX_BUTTON_SVG_HEIGHT / 2
            },
            color: MIN_MAX_BUTTON_COLOR,
            alpha: MIN_MAX_BUTTON_ALPHA,
            visible: false
        });

        scrollbarBackgroundPosition = {
            x: 0,
            y: viewport.y
        };
        scrollbarBackground = Overlays.addOverlay("text", {
            x: 0,
            y: scrollbarBackgroundPosition.y,
            width: SCROLLBAR_BACKGROUND_WIDTH,
            height: windowTextHeight,
            backgroundColor: SCROLLBAR_BACKGROUND_COLOR,
            backgroundAlpha: SCROLLBAR_BACKGROUND_ALPHA,
            text: "",
            visible: false
        });

        scrollbarBarPosition = {
            x: 0,
            y: viewport.y
        };
        scrollbarBar = Overlays.addOverlay("text", {
            x: 0,
            y: scrollbarBarPosition.y,
            width: SCROLLBAR_BACKGROUND_WIDTH - 2,
            height: windowTextHeight,
            backgroundColor: SCROLLBAR_BAR_COLOR,
            backgroundAlpha: SCROLLBAR_BAR_ALPHA,
            text: "",
            visible: false
        });

        friendsButton = Overlays.addOverlay("image", {
            x: 0,
            y: viewport.y,
            width: FRIENDS_BUTTON_WIDTH,
            height: FRIENDS_BUTTON_HEIGHT,
            imageURL: FRIENDS_BUTTON_SVG,
            subImage: {
                x: 0,
                y: 0,
                width: FRIENDS_BUTTON_SVG_WIDTH,
                height: FRIENDS_BUTTON_SVG_HEIGHT
            },
            color: FRIENDS_BUTTON_COLOR,
            alpha: FRIENDS_BUTTON_ALPHA,
            visible: false
        });

        showMe = Settings.getValue(SETTING_USERS_SHOW_ME, "");
        if (DISPLAY_VALUES.indexOf(showMe) === -1) {
            showMe = DISPLAY_EVERYONE;
        }

        displayControl = new PopUpMenu({
            prompt: DISPLAY_PROMPT,
            value: showMe,
            values: DISPLAY_VALUES,
            displayValues: DISPLAY_DISPLAY_VALUES,
            x: 0,
            y: viewport.y,
            width: WINDOW_WIDTH - 1.5 * WINDOW_MARGIN,
            promptWidth: DISPLAY_PROMPT_WIDTH,
            lineHeight: windowLineHeight,
            textHeight: windowTextHeight,
            font: WINDOW_FONT,
            promptColor: WINDOW_HEADING_COLOR,
            promptAlpha: WINDOW_HEADING_ALPHA,
            promptBackgroundColor: WINDOW_BACKGROUND_COLOR,
            promptBackgroundAlpha: 0.0,
            optionColor: WINDOW_FOREGROUND_COLOR,
            optionAlpha: WINDOW_FOREGROUND_ALPHA,
            optionBackgroundColor: OPTION_BACKGROUND_COLOR,
            optionBackgroundAlpha: OPTION_BACKGROUND_ALPHA,
            popupBackgroundColor: DISPLAY_OPTIONS_BACKGROUND_COLOR,
            popupBackgroundAlpha: DISPLAY_OPTIONS_BACKGROUND_ALPHA,
            buttonColor: MIN_MAX_BUTTON_COLOR,
            buttonAlpha: MIN_MAX_BUTTON_ALPHA,
            visible: false
        });

        myVisibility = Settings.getValue(SETTING_USERS_VISIBLE_TO, "");
        if (VISIBILITY_VALUES.indexOf(myVisibility) === -1) {
            myVisibility = VISIBILITY_FRIENDS;
        }

        visibilityControl = new PopUpMenu({
            prompt: VISIBILITY_PROMPT,
            value: myVisibility,
            values: VISIBILITY_VALUES,
            displayValues: VISIBILITY_DISPLAY_VALUES,
            x: 0,
            y: viewport.y,
            width: WINDOW_WIDTH - 1.5 * WINDOW_MARGIN,
            promptWidth: VISIBILITY_PROMPT_WIDTH,
            lineHeight: windowLineHeight,
            textHeight: windowTextHeight,
            font: WINDOW_FONT,
            promptColor: WINDOW_HEADING_COLOR,
            promptAlpha: WINDOW_HEADING_ALPHA,
            promptBackgroundColor: WINDOW_BACKGROUND_COLOR,
            promptBackgroundAlpha: 0.0,
            optionColor: WINDOW_FOREGROUND_COLOR,
            optionAlpha: WINDOW_FOREGROUND_ALPHA,
            optionBackgroundColor: OPTION_BACKGROUND_COLOR,
            optionBackgroundAlpha: OPTION_BACKGROUND_ALPHA,
            popupBackgroundColor: DISPLAY_OPTIONS_BACKGROUND_COLOR,
            popupBackgroundAlpha: DISPLAY_OPTIONS_BACKGROUND_ALPHA,
            buttonColor: MIN_MAX_BUTTON_COLOR,
            buttonAlpha: MIN_MAX_BUTTON_ALPHA,
            visible: false
        });

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

        GlobalServices.findableByChanged.connect(onFindableByChanged);

        Script.update.connect(onScriptUpdate);

        pollUsers();

        // Set minimized at end - setup code does not handle `minimized == false` correctly
        setMinimized(isValueTrue(Settings.getValue(SETTING_USERS_WINDOW_MINIMIZED, false)));
    }

    function tearDown() {
        Menu.removeMenuItem(MENU_NAME, MENU_ITEM);

        Script.clearTimeout(usersTimer);
        Overlays.deleteOverlay(windowBorder);
        Overlays.deleteOverlay(windowPane);
        Overlays.deleteOverlay(windowHeading);
        Overlays.deleteOverlay(minimizeButton);
        Overlays.deleteOverlay(scrollbarBackground);
        Overlays.deleteOverlay(scrollbarBar);
        Overlays.deleteOverlay(friendsButton);
        displayControl.tearDown();
        visibilityControl.tearDown();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());

}()); // END LOCAL_SCOPE
