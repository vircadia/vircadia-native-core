//
//  Created by Thijs Wenker on 3/31/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Revision of James B. Pollack's work on GamesTable in 2016
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// TODO: This game needs some work to get running
(function() {
    var _this;
    var MAPPING_NAME = "hifi-gametable-cards-dev-" + Math.random();
    var PLAYING_CARD_SCRIPT_URL = Script.resolvePath('playingCard.js');
    var PLAYING_CARD_MODEL_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/thoys/production/gameTable/assets/deckOfCards/playing_card.fbx");
    var PLAYING_CARD_BACK_IMAGE_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/thoys/production/gameTable/assets/deckOfCards/back.jpg");
    var PLAYING_CARD_DIMENSIONS = {
        x: 0.2621,
        y: 0.1,
        z: 0.4533
    };


    var COLORS_CAN_PLACE = {
        red: 0,
        green: 255,
        blue: 0
    };

    var COLORS_CANNOT_PLACE = {
        red: 255,
        green: 0,
        blue: 0
    };

    var NEARBY_CARDS_RANGE = 5;

    function DeckOfCards() {
        _this = this;
    }

    DeckOfCards.prototype = {
        rightOverlayLine: null,
        leftOverlayLine: null,
        targetOverlay: null,
        currentHand: null,
        offHand: null,
        updateConnected: null,
        preload: function(entityID) {
            print('jbp preload deck of cards');
            _this.entityID = entityID;
        },
        createMapping: function() {
            var mapping = Controller.newMapping(MAPPING_NAME);
            mapping.from([Controller.Standard.RTClick]).peek().to(function(val) {
                _this.handleTrigger(val, 'right');
            });
            mapping.from([Controller.Standard.LTClick]).peek().to(function(val) {
                _this.handleTrigger(val, 'left');
            });
            Controller.enableMapping(MAPPING_NAME);
        },
        destroyMapping: function() {
            Controller.disableMapping(MAPPING_NAME);
        },
        handleTrigger: function(val, hand) {
            if (val !== 1) {
                return;
            }
            print('jbp trigger pulled at val:' + val + ":" + hand);
            print('jbp at time hand was:' + _this.currentHand);
            if (_this.currentHand === hand) {
                print('jbp should ignore its the same hand');
            } else {
                print('jbp should make a new thing its the off hand');
                _this.createPlayingCard();
            }
        },
        checkIfAlreadyHasCards: function() {
            print('jbp checking if already has cards');
            var cards = _this.getCardsFromUserData();
            if (cards === false) {
                print('jbp should make new deck');
                // should make a deck the first time
                _this.makeNewDeck();
            } else {
                print('jbp already has deck' + cards);
                // someone already started a game with this deck
                _this.makeNewDeck();
                _this.currentStack._import(cards);
            }
        },

        resetDeck: function() {
            print('jbp resetting deck');
            // finds and delete any nearby cards
            var position = Entities.getEntityProperties(_this.entityID, 'position');
            var results = Entities.findEntities(position, NEARBY_CARDS_RANGE);
            results.forEach(function(item) {
                var itemProps = Entities.getEntityProperties(item, 'userData');
                if (itemProps.userData.hasOwnProperty('playingCards') &&
                    itemProps.userData.playingCards.hasOwnProperty('card')) {

                    Entities.deleteEntity(item);
                }
            });
            // resets this deck to a new deck
            _this.makeNewDeck();
        },

        makeNewDeck: function() {
            print('jbp make new deck');
            // make a stack and shuffle it up.
            var stack = new Stack();
            stack.makeDeck(1);
            stack.shuffle(100);
            _this.currentStack = stack;
        },

        collisionWithEntity: function(me, other, collision) {
            // on the start of a collision with the deck, if its a card, add it back to the deck
            if (collision.type !== 0) {
                // its not the start, so exit early.
                return;
            }
            var otherProps = Entities.getEntityProperties(other, 'userData');
            var userData = {};
            try {
                userData = JSON.parse(otherProps.userData);
            } catch (e) {
                return;
            }
            if (userData.hasOwnProperty('playingCards') && userData.playingCards.hasOwnProperty('card')) {
                print('collided with a playing card!!!');

                _this.currentStack.addCard(userData.playingCards.card);

                Entities.deleteEntity(other);
            }
        },

        startNearGrab: function(id, paramsArray) {
            print('jbp deck started near grab');
            _this.checkIfAlreadyHasCards();
            _this.enterDealerMode(paramsArray[0]);
        },
        startEquip: function(id, params) {
            this.startNearGrab(id, params);
        },

        releaseGrab: function() {
            print('jbp release grab');
            _this.exitDealerMode();
        },

        enterDealerMode: function(hand) {
            _this.createMapping();
            print('jbp enter dealer mode:' + hand);
            var offHand;
            if (hand === "left") {
                offHand = "right";
            }
            if (hand === "right") {
                offHand = "left";
            }
            _this.currentHand = hand;
            _this.offHand = offHand;
            Messages.sendLocalMessage('Hifi-Hand-Disabler', _this.offHand);
            Script.update.connect(_this.updateRays);
            _this.updateConnected = true;
        },

        updateRays: function() {
            if (_this.currentHand === 'left') {
                _this.rightRay();
            }
            if (_this.currentHand === 'right') {
                _this.leftRay();
            }
        },

        exitDealerMode: function() {
            _this.destroyMapping();
            // turn grab on
            // save the cards
            // delete the overlay beam
            if (_this.updateConnected === true) {
                Script.update.disconnect(_this.updateRays);
            }
            Messages.sendLocalMessage('Hifi-Hand-Disabler', 'none');
            _this.deleteCardTargetOverlay();
            _this.turnOffOverlayBeams();
            _this.storeCards();
        },

        storeCards: function() {
            var cards = _this.currentStack._export();
            print('deck of cards: ' + cards);
            Entities.editEntity(_this.entityID, {
                userData: JSON.stringify({
                    cards: cards
                })
            });
        },

        restoreCards: function() {
            _this.currentStack = _this.getCardsFromUserData();

        },

        getCardsFromUserData: function() {
            var props = Entities.getEntityProperties(_this.entityID, 'userData');
            var data;
            try {
                data = JSON.parse(props.userData);
                print('jbp has cards in userdata' + props.userData);

            } catch (e) {
                print('jbp error parsing userdata');
                return false;
            }
            if (data.hasOwnProperty('cards')) {
                print('jbp returning data.cards');
                return data.cards;
            }
            return false;
        },

        rightRay: function() {
            var pose = Controller.getPoseValue(Controller.Standard.RightHand);
            var rightPosition = pose.valid ?
                Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position) :
                MyAvatar.getHeadPosition();
            var rightRotation = pose.valid ? Quat.multiply(MyAvatar.orientation, pose.rotation) :
                Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, {
                    x: 1,
                    y: 0,
                    z: 0
                }));

            var rightPickRay = {
                origin: rightPosition,
                direction: Quat.getUp(rightRotation)
            };

            this.rightPickRay = rightPickRay;

            var location = Vec3.sum(rightPickRay.origin, Vec3.multiply(rightPickRay.direction, 50));

            var rightIntersection = Entities.findRayIntersection(rightPickRay, true, [], [this.targetEntity]);

            if (rightIntersection.intersects) {
                this.rightLineOn(rightPickRay.origin, rightIntersection.intersection, COLORS_CAN_PLACE);

                if (this.targetOverlay !== null) {
                    this.updateTargetOverlay(rightIntersection);
                } else {
                    this.createTargetOverlay();
                }
            } else {
                this.rightLineOn(rightPickRay.origin, location, COLORS_CANNOT_PLACE);
            }
        },
        leftRay: function() {
            var pose = Controller.getPoseValue(Controller.Standard.LeftHand);
            var leftPosition = pose.valid ? Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position) : MyAvatar.getHeadPosition();
            var leftRotation = pose.valid ? Quat.multiply(MyAvatar.orientation, pose.rotation) :
                Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, {
                    x: 1,
                    y: 0,
                    z: 0
                }));

            var leftPickRay = {
                origin: leftPosition,
                direction: Quat.getUp(leftRotation)
            };

            this.leftPickRay = leftPickRay;

            var location = Vec3.sum(MyAvatar.position, Vec3.multiply(leftPickRay.direction, 50));

            var leftIntersection = Entities.findRayIntersection(leftPickRay, true, [], [this.targetEntity]);

            if (leftIntersection.intersects) {
                this.leftLineOn(leftPickRay.origin, leftIntersection.intersection, COLORS_CAN_PLACE);
                if (this.targetOverlay !== null) {
                    this.updateTargetOverlay(leftIntersection);
                } else {
                    this.createTargetOverlay();
                }

            } else {
                this.leftLineOn(leftPickRay.origin, location, COLORS_CANNOT_PLACE);
            }
        },
        rightLineOn: function(closePoint, farPoint, color) {
            if (this.rightOverlayLine === null) {
                var lineProperties = {
                    start: closePoint,
                    end: farPoint,
                    color: color,
                    ignoreRayIntersection: true,
                    visible: true,
                    alpha: 1,
                    solid: true,
                    drawInFront: true,
                    glow: 1.0
                };

                this.rightOverlayLine = Overlays.addOverlay("line3d", lineProperties);

            } else {
                Overlays.editOverlay(this.rightOverlayLine, {
                    start: closePoint,
                    end: farPoint,
                    color: color
                });
            }
        },
        leftLineOn: function(closePoint, farPoint, color) {
            if (this.leftOverlayLine === null) {
                var lineProperties = {
                    ignoreRayIntersection: true,
                    start: closePoint,
                    end: farPoint,
                    color: color,
                    visible: true,
                    alpha: 1,
                    solid: true,
                    glow: 1.0,
                    drawInFront: true
                };

                this.leftOverlayLine = Overlays.addOverlay("line3d", lineProperties);

            } else {
                Overlays.editOverlay(this.leftOverlayLine, {
                    start: closePoint,
                    end: farPoint,
                    color: color
                });
            }
        },
        rightOverlayOff: function() {
            if (this.rightOverlayLine !== null) {
                print('jbp inside right off');
                Overlays.deleteOverlay(this.rightOverlayLine);
                this.rightOverlayLine = null;
            }
        },
        leftOverlayOff: function() {
            if (this.leftOverlayLine !== null) {
                print('jbp inside left off');
                Overlays.deleteOverlay(this.leftOverlayLine);
                this.leftOverlayLine = null;
            }
        },
        turnOffOverlayBeams: function() {
            this.rightOverlayOff();
            this.leftOverlayOff();
        },
        createTargetOverlay: function() {
            print('jbp should create target overlay');
            if (_this.targetOverlay !== null) {
                return;
            }
            var targetOverlayProps = {
                url: PLAYING_CARD_MODEL_URL,
                dimensions: PLAYING_CARD_DIMENSIONS,
                position: MyAvatar.position,
                visible: true
            };


            _this.targetOverlay = Overlays.addOverlay("model", targetOverlayProps);

            print('jbp created target overlay: ' + _this.targetOverlay);
        },

        updateTargetOverlay: function(intersection) {
            // print('jbp should update target overlay: ' + _this.targetOverlay)
            _this.intersection = intersection;

            var rotation = Quat.lookAt(intersection.intersection, MyAvatar.position, Vec3.UP);
            var euler = Quat.safeEulerAngles(rotation);
            var position = {
                x: intersection.intersection.x,
                y: intersection.intersection.y + PLAYING_CARD_DIMENSIONS.y / 2,
                z: intersection.intersection.z
            };

            _this.shiftedIntersectionPosition = position;

            var towardUs = Quat.fromPitchYawRollDegrees(0, euler.y, 0);
            // print('jbp should update target overlay to ' + JSON.stringify(position))
            Overlays.editOverlay(_this.targetOverlay, {
                position: position,
                rotation: towardUs
            });

        },

        deleteCardTargetOverlay: function() {
            Overlays.deleteOverlay(_this.targetOverlay);
            _this.targetOverlay = null;
        },
        handleEndOfDeck: function() {
            print('jbp at the end of the deck, no more.');
        },

        createPlayingCard: function() {
            print('jbp should create playing card');
            if (_this.currentStack.cards.length > 0) {
                var card = _this.currentStack.draw(1);
            } else {
                _this.handleEndOfDeck();
                return;
            }

            print('jbp drew card: ' + card);
            var properties = {
                type: 'Model',
                description: 'hifi:gameTable:game:playingCards',
                dimensions: PLAYING_CARD_DIMENSIONS,
                modelURL: PLAYING_CARD_MODEL_URL,
                script: PLAYING_CARD_SCRIPT_URL,
                position: _this.shiftedIntersectionPosition,
                shapeType: "box",
                dynamic: true,
                gravity: {
                    x: 0,
                    y: -9.8,
                    z: 0
                },
                textures: JSON.stringify({
                    file1: PLAYING_CARD_BACK_IMAGE_URL,
                    file2: PLAYING_CARD_BACK_IMAGE_URL
                }),
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: true
                    },
                    playingCards: {
                        card: card
                    }
                })
            };
            Entities.addEntity(properties);
        }

    };

    function Card(rank, suit) {
        this.rank = rank;
        this.suit = suit;
    }

    function stackImportCards(exportedCards) {
        print('jbp importing ' + exportedCards);
        var cards = JSON.parse(exportedCards);
        this.cards = [];
        var cardArray = this.cards;
        cards.forEach(function(card) {
            var newCard = new Card(card.substr(1, card.length), card[0]);
            cardArray.push(newCard);
        });
    }

    function stackExportCards() {
        var _export = [];
        this.cards.forEach(function(item) {
            _export.push(item.suit + item.rank);
        });
        return JSON.stringify(_export);
    }

    function stackAddCard(card) {

        this.cards.push(card);
    }

    function stackMakeDeck(n) {

        var ranks = ["A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"];
        var suits = ["C", "D", "H", "S"];
        var i, j, k;
        var m;

        m = ranks.length * suits.length;

        // Set array of cards.

        this.cards = [];

        // Fill the array with 'n' packs of cards.

        for (i = 0; i < n; i++) {
            for (j = 0; j < suits.length; j++) {
                for (k = 0; k < ranks.length; k++) {
                    this.cards[i * m + j * ranks.length + k] = new Card(ranks[k], suits[j]);
                }
            }
        }
    }

    function stackShuffle(n) {

        var i, j, k;
        var temp;

        // Shuffle the stack 'n' times.

        for (i = 0; i < n; i++) {
            for (j = 0; j < this.cards.length; j++) {
                k = Math.floor(Math.random() * this.cards.length);
                temp = this.cards[j];
                this.cards[j] = this.cards[k];
                this.cards[k] = temp;
            }
        }
    }

    function stackDeal() {
        if (this.cards.length > 0) {
            return this.cards.shift();
        }
        return null;
    }

    function stackDraw(n) {

        var card;

        if (n >= 0 && n < this.cards.length) {
            card = this.cards[n];
            this.cards.splice(n, 1);
        } else {
            card = null;
        }

        return card;
    }

    function stackCardCount() {

        return this.cards.length;
    }

    function stackCombine(stack) {

        this.cards = this.cards.concat(stack.cards);
        stack.cards = [];
    }

    function Stack() {

        // Create an empty array of cards.

        this.cards = [];

        this.makeDeck = stackMakeDeck;
        this.shuffle = stackShuffle;
        this.deal = stackDeal;
        this.draw = stackDraw;
        this.addCard = stackAddCard;
        this.combine = stackCombine;
        this.cardCount = stackCardCount;
        this._export = stackExportCards;
        this._import = stackImportCards;
    }


    return new DeckOfCards();
});