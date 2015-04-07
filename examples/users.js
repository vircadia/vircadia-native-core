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

    var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/",

        WINDOW_WIDTH = 160,
        WINDOW_MARGIN = 12,
        WINDOW_BASE_MARGIN = 6,                             // A little less is needed in order look correct
        WINDOW_FONT = { size: 12 },
        WINDOW_FOREGROUND_COLOR = { red: 240, green: 240, blue: 240 },
        WINDOW_FOREGROUND_ALPHA = 0.9,
        WINDOW_HEADING_COLOR = { red: 180, green: 180, blue: 180 },
        WINDOW_HEADING_ALPHA = 0.9,
        WINDOW_BACKGROUND_COLOR = { red: 80, green: 80, blue: 80 },
        WINDOW_BACKGROUND_ALPHA = 0.7,
        windowPane,
        windowHeading,
        MIN_MAX_BUTTON_SVG = HIFI_PUBLIC_BUCKET + "images/tools/min-max-toggle.svg",
        MIN_MAX_BUTTON_SVG_WIDTH = 17.1,
        MIN_MAX_BUTTON_SVG_HEIGHT = 32.5,
        MIN_MAX_BUTTON_WIDTH = 14,
        MIN_MAX_BUTTON_HEIGHT = MIN_MAX_BUTTON_WIDTH,
        MIN_MAX_BUTTON_COLOR = { red: 255, green: 255, blue: 255 },
        MIN_MAX_BUTTON_ALPHA = 0.9,
        minimizeButton,
        SCROLLBAR_BACKGROUND_WIDTH = 12,
        SCROLLBAR_BACKGROUND_COLOR = { red: 70, green: 70, blue: 70 },
        SCROLLBAR_BACKGROUND_ALPHA = 0.8,
        scrollbarBackground,
        SCROLLBAR_BAR_MIN_HEIGHT = 5,
        SCROLLBAR_BAR_COLOR = { red: 170, green: 170, blue: 170 },
        SCROLLBAR_BAR_ALPHA = 0.8,
        SCROLLBAR_BAR_SELECTED_ALPHA = 0.9,
        scrollbarBar,
        scrollbarBackgroundHeight,
        scrollbarBarHeight,
        FRIENDS_BUTTON_SPACER = 12,                         // Space before add/remove friends button
        FRIENDS_BUTTON_SVG = HIFI_PUBLIC_BUCKET + "images/tools/add-remove-friends.svg",
        FRIENDS_BUTTON_SVG_WIDTH = 107,
        FRIENDS_BUTTON_SVG_HEIGHT = 27,
        FRIENDS_BUTTON_WIDTH = FRIENDS_BUTTON_SVG_WIDTH,
        FRIENDS_BUTTON_HEIGHT = FRIENDS_BUTTON_SVG_HEIGHT,
        FRIENDS_BUTTON_COLOR = { red: 255, green: 255, blue: 255 },
        FRIENDS_BUTTON_ALPHA = 0.9,
        friendsButton,

        OPTION_BACKGROUND_COLOR = { red: 60, green: 60, blue: 60 },
        OPTION_BACKGROUND_ALPHA = 0.8,
        OPTION_MARGIN = 4,
        DISPLAY_SPACER = 12,                                // Space before display control
        DISPLAY_PROMPT_TEXT = "Show me:",
        DISPLAY_PROMPT_WIDTH = 60,
        displayValues = ["everyone", "friends"],
        displayControl,
        DISPLAY_OPTIONS_BACKGROUND_COLOR = { red: 40, green: 40, blue: 40 },
        DISPLAY_OPTIONS_BACKGROUND_ALPHA = 0.95,
        displayOptions = [],
        isShowingDisplayOptions = false,

        VISIBILITY_SPACER = 6,                             // Space before visibility controls
        visibilityHeading,
        VISIBILITY_RADIO_SPACE = 16,
        visibilityControls,
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

        MENU_NAME = "Tools",
        MENU_ITEM = "Users Online",
        MENU_ITEM_AFTER = "Chat...",

        isVisible = true,
        isMinimized = false,

        viewportHeight,
        isMirrorDisplay = false,
        isFullscreenMirror = false,

        isUsingScrollbars = false,
        isMovingScrollbar = false,
        scrollbarBackgroundPosition = {},
        scrollbarBarPosition = {},
        scrollbarBarClickedAt,                              // 0.0 .. 1.0
        scrollbarValue = 0.0,                               // 0.0 .. 1.0

        RADIO_BUTTON_SVG = HIFI_PUBLIC_BUCKET + "images/radio-button.svg",
        RADIO_BUTTON_SVG_DIAMETER = 14,
        RADIO_BUTTON_DISPLAY_SCALE = 0.7,                   // 1.0 = windowTextHeight
        radioButtonDiameter;

    function calculateWindowHeight() {
        var AUDIO_METER_HEIGHT = 52,
            MIRROR_HEIGHT = 220,
            nonUsersHeight,
            maxWindowHeight;

        if (isMinimized) {
            windowHeight = windowTextHeight + WINDOW_MARGIN + WINDOW_BASE_MARGIN;
            return;
        }

        // Reserve 5 lines for window heading plus display and visibility controls
        // Subtract windowLineSpacing for both end of user list and end of controls
        nonUsersHeight = 6 * windowLineHeight - 2 * windowLineSpacing
            + FRIENDS_BUTTON_SPACER + FRIENDS_BUTTON_HEIGHT
            + DISPLAY_SPACER + VISIBILITY_SPACER + WINDOW_MARGIN + WINDOW_BASE_MARGIN;

        // Limit window to height of viewport minus VU meter and mirror if displayed
        windowHeight = linesOfUsers.length * windowLineHeight - windowLineSpacing + nonUsersHeight;
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

    function deleteDisplayOptions() {
        var i;

        for (i = 0; i < displayOptions.length; i += 1) {
            Overlays.deleteOverlay(displayOptions[i]);
        }

        isShowingDisplayOptions = false;
    }

    function positionDisplayOptions() {
        var y,
            i;

        y = viewportHeight
                - windowTextHeight
                - VISIBILITY_SPACER - 4 * windowLineHeight + windowLineSpacing - WINDOW_BASE_MARGIN
                - (displayValues.length - 1) * windowLineHeight;
        for (i = 0; i < displayValues.length; i += 1) {
            Overlays.editOverlay(displayOptions[i], { y: y });
            y += windowLineHeight;
        }
    }

    function showDisplayOptions() {
        var i;

        for (i = 0; i < displayValues.length; i += 1) {
            displayOptions[i] = Overlays.addOverlay("text", {
                x: WINDOW_MARGIN + DISPLAY_PROMPT_WIDTH,
                y: viewportHeight,
                width: WINDOW_WIDTH - 1.5 * WINDOW_MARGIN - DISPLAY_PROMPT_WIDTH,
                height: windowTextHeight + OPTION_MARGIN,  // Only need to add margin at top to balance descenders
                topMargin: OPTION_MARGIN,
                leftMargin: OPTION_MARGIN,
                color: WINDOW_FOREGROUND_COLOR,
                alpha: WINDOW_FOREGROUND_ALPHA,
                backgroundColor: DISPLAY_OPTIONS_BACKGROUND_COLOR,
                backgroundAlpha: DISPLAY_OPTIONS_BACKGROUND_ALPHA,
                text: displayValues[i],
                font: WINDOW_FONT,
                visible: true
            });
        }

        positionDisplayOptions();

        isShowingDisplayOptions = true;
    }

    function updateOverlayPositions() {
        var i,
            y;

        Overlays.editOverlay(windowPane, {
            y: viewportHeight - windowHeight
        });
        Overlays.editOverlay(windowHeading, {
            y: viewportHeight - windowHeight + WINDOW_MARGIN
        });

        Overlays.editOverlay(minimizeButton, {
            y: viewportHeight - windowHeight + WINDOW_MARGIN / 2
        });

        scrollbarBackgroundPosition.y = viewportHeight - windowHeight + WINDOW_MARGIN + windowTextHeight;
        Overlays.editOverlay(scrollbarBackground, {
            y: scrollbarBackgroundPosition.y
        });
        scrollbarBarPosition.y = scrollbarBackgroundPosition.y + 1
                + scrollbarValue * (scrollbarBackgroundHeight - scrollbarBarHeight - 2);
        Overlays.editOverlay(scrollbarBar, {
            y: scrollbarBarPosition.y
        });

        Overlays.editOverlay(friendsButton, {
            y: viewportHeight - FRIENDS_BUTTON_HEIGHT
                - DISPLAY_SPACER - windowTextHeight
                - VISIBILITY_SPACER - 4 * windowLineHeight + windowLineSpacing - WINDOW_BASE_MARGIN
        });

        y = viewportHeight
                - windowTextHeight
                - VISIBILITY_SPACER - 4 * windowLineHeight + windowLineSpacing - WINDOW_BASE_MARGIN;
        Overlays.editOverlay(displayControl.promptOverlay, { y: y });
        Overlays.editOverlay(displayControl.valueOverlay, { y: y - OPTION_MARGIN });
        Overlays.editOverlay(displayControl.buttonOverlay, { y: y - OPTION_MARGIN + 1 });

        if (isShowingDisplayOptions) {
            positionDisplayOptions();
        }

        Overlays.editOverlay(visibilityHeading, {
            y: viewportHeight - 4 * windowLineHeight + windowLineSpacing - WINDOW_BASE_MARGIN
        });
        for (i = 0; i < visibilityControls.length; i += 1) {
            y = viewportHeight - (3 - i) * windowLineHeight + windowLineSpacing - WINDOW_BASE_MARGIN;
            Overlays.editOverlay(visibilityControls[i].radioOverlay, { y: y });
            Overlays.editOverlay(visibilityControls[i].textOverlay, { y: y });
        }
    }

    function updateVisibilityControls() {
        var i,
            color;

        for (i = 0; i < visibilityControls.length; i += 1) {
            color = visibilityControls[i].selected ? WINDOW_FOREGROUND_COLOR : WINDOW_HEADING_COLOR;
            Overlays.editOverlay(visibilityControls[i].radioOverlay, {
                color: color,
                subImage: { y: visibilityControls[i].selected ? 0 : RADIO_BUTTON_SVG_DIAMETER }
            });
            Overlays.editOverlay(visibilityControls[i].textOverlay, { color: color });
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

            displayText = displayText.slice(1);  // Remove leading "\n".

            scrollbarBackgroundHeight = numUsersToDisplay * windowLineHeight - windowLineSpacing / 2;
            Overlays.editOverlay(scrollbarBackground, {
                height: scrollbarBackgroundHeight,
                visible: isUsingScrollbars
            });
            scrollbarBarHeight = Math.max(numUsersToDisplay / linesOfUsers.length * scrollbarBackgroundHeight,
                SCROLLBAR_BAR_MIN_HEIGHT);
            Overlays.editOverlay(scrollbarBar, {
                height: scrollbarBarHeight,
                visible: isUsingScrollbars
            });
        }

        Overlays.editOverlay(windowPane, {
            height: windowHeight,
            text: displayText
        });

        Overlays.editOverlay(windowHeading, {
            text: linesOfUsers.length > 0 ? "Users online" : "No users online"
        });
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
                        usersOnline[i].textWidth = Overlays.textSize(windowPane, userText).width;

                        linesOfUsers.push(i);
                    }
                }

                calculateWindowHeight();
                updateUsersDisplay();
                updateOverlayPositions();

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

    function updateOverlayVisibility() {
        var i;

        Overlays.editOverlay(windowPane, { visible: isVisible });
        Overlays.editOverlay(windowHeading, { visible: isVisible });
        Overlays.editOverlay(minimizeButton, { visible: isVisible });
        Overlays.editOverlay(scrollbarBackground, { visible: isVisible && isUsingScrollbars && !isMinimized });
        Overlays.editOverlay(scrollbarBar, { visible: isVisible && isUsingScrollbars && !isMinimized });
        Overlays.editOverlay(friendsButton, { visible: isVisible && !isMinimized });
        Overlays.editOverlay(displayControl.promptOverlay, { visible: isVisible && !isMinimized });
        Overlays.editOverlay(displayControl.valueOverlay, { visible: isVisible && !isMinimized });
        Overlays.editOverlay(displayControl.buttonOverlay, { visible: isVisible && !isMinimized });
        Overlays.editOverlay(visibilityHeading, { visible: isVisible && !isMinimized });
        for (i = 0; i < visibilityControls.length; i += 1) {
            Overlays.editOverlay(visibilityControls[i].radioOverlay, { visible: isVisible && !isMinimized });
            Overlays.editOverlay(visibilityControls[i].textOverlay, { visible: isVisible && !isMinimized });
        }
    }

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
            subImage: { y: isMinimized ? MIN_MAX_BUTTON_SVG_HEIGHT / 2 : 0 }
        });
        updateOverlayVisibility();
    }

    function onMenuItemEvent(event) {
        if (event === MENU_ITEM) {
            setVisible(Menu.isOptionChecked(MENU_ITEM));
        }
    }

    function onFindableByChanged(event) {
        var i;

        for (i = 0; i < visibilityControls.length; i += 1) {
            visibilityControls[i].selected = event === VISIBILITY_VALUES[i];
        }

        updateVisibilityControls();
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
            delta;

        if (!isVisible) {
            return;
        }

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (isShowingDisplayOptions) {
            userClicked = false;
            for (i = 0; i < displayOptions.length; i += 1) {
                if (clickedOverlay === displayOptions[i]) {
                    displayControl.value = displayValues[i];
                    Overlays.editOverlay(displayControl.valueOverlay, { text: displayValues[i] });
                    userClicked = true;
                }
            }

            deleteDisplayOptions();

            if (userClicked) {
                return;
            }
        }

        if (clickedOverlay === windowPane) {

            overlayX = event.x - WINDOW_MARGIN;
            overlayY = event.y - viewportHeight + windowHeight - WINDOW_MARGIN - windowLineHeight;

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

            return;
        }

        if (clickedOverlay === displayControl.valueOverlay || clickedOverlay === displayControl.buttonOverlay) {
            showDisplayOptions();
            return;
        }

        for (i = 0; i < visibilityControls.length; i += 1) {
            // Don't need to test radioOverlay if it us under textOverlay.
            if (clickedOverlay === visibilityControls[i].textOverlay && event.x <= visibilityControls[i].optionWidth) {
                GlobalServices.findableBy = VISIBILITY_VALUES[i];
                return;
            }
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
            GlobalServices.editFriends();
        }
    }

    function onMouseMoveEvent(event) {
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
    }

    function onMouseReleaseEvent() {
        Overlays.editOverlay(scrollbarBar, {
            backgroundAlpha: SCROLLBAR_BAR_ALPHA
        });
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

        textSizeOverlay = Overlays.addOverlay("text", { font: WINDOW_FONT, visible: false });
        windowTextHeight = Math.floor(Overlays.textSize(textSizeOverlay, "1").height);
        windowLineSpacing = Math.floor(Overlays.textSize(textSizeOverlay, "1\n2").height - 2 * windowTextHeight);
        windowLineHeight = windowTextHeight + windowLineSpacing;
        radioButtonDiameter = RADIO_BUTTON_DISPLAY_SCALE * windowTextHeight;
        Overlays.deleteOverlay(textSizeOverlay);

        viewportHeight = Controller.getViewportDimensions().y;

        calculateWindowHeight();

        windowPane = Overlays.addOverlay("text", {
            x: 0,
            y: viewportHeight,  // Start up off-screen
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
            visible: isVisible
        });

        windowHeading = Overlays.addOverlay("text", {
            x: WINDOW_MARGIN,
            y: viewportHeight,
            width: WINDOW_WIDTH - 2 * WINDOW_MARGIN,
            height: windowTextHeight,
            topMargin: 0,
            leftMargin: 0,
            color: WINDOW_HEADING_COLOR,
            alpha: WINDOW_HEADING_ALPHA,
            backgroundAlpha: 0.0,
            text: "No users online",
            font: WINDOW_FONT,
            visible: isVisible && !isMinimized
        });

        minimizeButton = Overlays.addOverlay("image", {
            x: WINDOW_WIDTH - WINDOW_MARGIN / 2 - MIN_MAX_BUTTON_WIDTH,
            y: viewportHeight,
            width: MIN_MAX_BUTTON_WIDTH,
            height: MIN_MAX_BUTTON_HEIGHT,
            imageURL: MIN_MAX_BUTTON_SVG,
            subImage: { x: 0, y: 0, width: MIN_MAX_BUTTON_SVG_WIDTH, height: MIN_MAX_BUTTON_SVG_HEIGHT / 2 },
            color: MIN_MAX_BUTTON_COLOR,
            alpha: MIN_MAX_BUTTON_ALPHA,
            visible:  isVisible && !isMinimized
        });

        scrollbarBackgroundPosition = {
            x: WINDOW_WIDTH - 0.5 * WINDOW_MARGIN - SCROLLBAR_BACKGROUND_WIDTH,
            y: viewportHeight
        };
        scrollbarBackground = Overlays.addOverlay("text", {
            x: scrollbarBackgroundPosition.x,
            y: scrollbarBackgroundPosition.y,
            width: SCROLLBAR_BACKGROUND_WIDTH,
            height: windowTextHeight,
            backgroundColor: SCROLLBAR_BACKGROUND_COLOR,
            backgroundAlpha: SCROLLBAR_BACKGROUND_ALPHA,
            text: "",
            visible: isVisible && isUsingScrollbars && !isMinimized
        });

        scrollbarBarPosition = {
            x: WINDOW_WIDTH - 0.5 * WINDOW_MARGIN - SCROLLBAR_BACKGROUND_WIDTH + 1,
            y: viewportHeight
        };
        scrollbarBar = Overlays.addOverlay("text", {
            x: scrollbarBarPosition.x,
            y: scrollbarBarPosition.y,
            width: SCROLLBAR_BACKGROUND_WIDTH - 2,
            height: windowTextHeight,
            backgroundColor: SCROLLBAR_BAR_COLOR,
            backgroundAlpha: SCROLLBAR_BAR_ALPHA,
            text: "",
            visible: isVisible && isUsingScrollbars && !isMinimized
        });

        friendsButton = Overlays.addOverlay("image", {
            x: WINDOW_MARGIN,
            y: viewportHeight,
            width: FRIENDS_BUTTON_WIDTH,
            height: FRIENDS_BUTTON_HEIGHT,
            imageURL: FRIENDS_BUTTON_SVG,
            subImage: { x: 0, y: 0, width: FRIENDS_BUTTON_SVG_WIDTH, height: FRIENDS_BUTTON_SVG_HEIGHT },
            color: FRIENDS_BUTTON_COLOR,
            alpha: FRIENDS_BUTTON_ALPHA
        });

        displayControl = {
            value: displayValues[0],
            promptOverlay: Overlays.addOverlay("text", {
                x: WINDOW_MARGIN,
                y: viewportHeight,
                width: DISPLAY_PROMPT_WIDTH,
                height: windowTextHeight,
                topMargin: 0,
                leftMargin: 0,
                color: WINDOW_HEADING_COLOR,
                alpha: WINDOW_HEADING_ALPHA,
                backgroundAlpha: 0.0,
                text: DISPLAY_PROMPT_TEXT,
                font: WINDOW_FONT,
                visible: isVisible && !isMinimized
            }),
            valueOverlay: Overlays.addOverlay("text", {
                x: WINDOW_MARGIN + DISPLAY_PROMPT_WIDTH,
                y: viewportHeight,
                width: WINDOW_WIDTH - 1.5 * WINDOW_MARGIN - DISPLAY_PROMPT_WIDTH,
                height: windowTextHeight + OPTION_MARGIN,  // Only need to add margin at top to balance descenders
                topMargin: OPTION_MARGIN,
                leftMargin: OPTION_MARGIN,
                color: WINDOW_FOREGROUND_COLOR,
                alpha: WINDOW_FOREGROUND_ALPHA,
                backgroundColor: OPTION_BACKGROUND_COLOR,
                backgroundAlpha: OPTION_BACKGROUND_ALPHA,
                text: displayValues[0],
                font: WINDOW_FONT,
                visible: isVisible && !isMinimized
            }),
            buttonOverlay: Overlays.addOverlay("image", {
                x: WINDOW_WIDTH - WINDOW_MARGIN / 2 - MIN_MAX_BUTTON_WIDTH - 1,
                y: viewportHeight,
                width: MIN_MAX_BUTTON_WIDTH,
                height: MIN_MAX_BUTTON_HEIGHT,
                imageURL: MIN_MAX_BUTTON_SVG,
                subImage: { x: 0, y: 0, width: MIN_MAX_BUTTON_SVG_WIDTH, height: MIN_MAX_BUTTON_SVG_HEIGHT / 2 },
                color: MIN_MAX_BUTTON_COLOR,
                alpha: MIN_MAX_BUTTON_ALPHA,
                visible: isVisible && !isMinimized
            })
        };

        visibilityHeading = Overlays.addOverlay("text", {
            x: WINDOW_MARGIN,
            y: viewportHeight,
            width: WINDOW_WIDTH - 2 * WINDOW_MARGIN,
            height: windowTextHeight,
            topMargin: 0,
            leftMargin: 0,
            color: WINDOW_HEADING_COLOR,
            alpha: WINDOW_HEADING_ALPHA,
            backgroundAlpha: 0.0,
            text: "I am visible to:",
            font: WINDOW_FONT,
            visible: isVisible && !isMinimized
        });

        myVisibility = GlobalServices.findableBy;
        if (!/^(all|friends|none)$/.test(myVisibility)) {
            print("Error: Unrecognized findableBy value");
            myVisibility = "";
        }

        optionText = "everyone";
        visibilityControls = [{
            radioOverlay: Overlays.addOverlay("image", {  // Create first so that it is under textOverlay.
                x: WINDOW_MARGIN,
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
                color: WINDOW_HEADING_COLOR,
                alpha: WINDOW_FOREGROUND_ALPHA,
                visible: isVisible && !isMinimized
            }),
            textOverlay: Overlays.addOverlay("text", {
                x: WINDOW_MARGIN,
                y: viewportHeight,
                width: WINDOW_WIDTH - SCROLLBAR_BACKGROUND_WIDTH - 2 * WINDOW_MARGIN,
                height: windowTextHeight,
                topMargin: 0,
                leftMargin: VISIBILITY_RADIO_SPACE,
                color: WINDOW_HEADING_COLOR,
                alpha: WINDOW_FOREGROUND_ALPHA,
                backgroundAlpha: 0.0,
                text: optionText,
                font: WINDOW_FONT,
                visible: isVisible && !isMinimized
            }),
            selected: myVisibility === VISIBILITY_VALUES[0]
        }];
        visibilityControls[0].optionWidth = WINDOW_MARGIN + VISIBILITY_RADIO_SPACE
            + Overlays.textSize(visibilityControls[0].textOverlay, optionText).width;

        optionText = "my friends";
        visibilityControls[1] = {
            radioOverlay: Overlays.cloneOverlay(visibilityControls[0].radioOverlay),
            textOverlay: Overlays.cloneOverlay(visibilityControls[0].textOverlay),
            selected: myVisibility === VISIBILITY_VALUES[1]
        };
        Overlays.editOverlay(visibilityControls[1].textOverlay, { text: optionText });
        visibilityControls[1].optionWidth = WINDOW_MARGIN + VISIBILITY_RADIO_SPACE
            + Overlays.textSize(visibilityControls[1].textOverlay, optionText).width;

        optionText = "no one";
        visibilityControls[2] = {
            radioOverlay: Overlays.cloneOverlay(visibilityControls[0].radioOverlay),
            textOverlay: Overlays.cloneOverlay(visibilityControls[0].textOverlay),
            selected: myVisibility === VISIBILITY_VALUES[2]
        };
        Overlays.editOverlay(visibilityControls[2].textOverlay, { text: optionText });
        visibilityControls[2].optionWidth = WINDOW_MARGIN + VISIBILITY_RADIO_SPACE
            + Overlays.textSize(visibilityControls[2].textOverlay, optionText).width;

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

        GlobalServices.findableByChanged.connect(onFindableByChanged);

        Script.update.connect(onScriptUpdate);

        pollUsers();

    }

    function tearDown() {
        var i;

        Menu.removeMenuItem(MENU_NAME, MENU_ITEM);

        Script.clearTimeout(usersTimer);
        Overlays.deleteOverlay(windowPane);
        Overlays.deleteOverlay(windowHeading);
        Overlays.deleteOverlay(minimizeButton);
        Overlays.deleteOverlay(scrollbarBackground);
        Overlays.deleteOverlay(scrollbarBar);
        Overlays.deleteOverlay(friendsButton);
        Overlays.deleteOverlay(displayControl.promptOverlay);
        Overlays.deleteOverlay(displayControl.valueOverlay);
        Overlays.deleteOverlay(displayControl.buttonOverlay);
        print("isShowingDisplayOptions = " + isShowingDisplayOptions);
        if (isShowingDisplayOptions) {
            deleteDisplayOptions();
        }
        Overlays.deleteOverlay(visibilityHeading);
        for (i = 0; i <= visibilityControls.length; i += 1) {
            Overlays.deleteOverlay(visibilityControls[i].textOverlay);
            Overlays.deleteOverlay(visibilityControls[i].radioOverlay);
        }
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
