(function() {

    var _this;

    var CARD_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/thoys/production/gameTable/assets/deckOfCards/playing_card.fbx';
    var CARD_BACK_IMAGE_URL = "http://hifi-content.s3.amazonaws.com/thoys/production/gameTable/assets/deckOfCards/images/back.jpg";
    var CARD_IMAGE_BASE_URL = "http://hifi-content.s3.amazonaws.com/thoys/production/gameTable/assets/deckOfCards/images/playingcard_old-";

    function PlayingCard() {
        _this = this;
    }

    PlayingCard.prototype = {
        preload: function(id) {
            _this.entityID = id;
        },
        startDistanceGrab:function(){
             _this.attachCardOverlay();
        },
        startNearGrab: function() {
            _this.attachCardOverlay();
        },
        releaseGrab: function() {
            _this.detachCardOverlay();
        },
        attachCardOverlay: function() {
            print('jbp should attach card overlay')
            var myProps = Entities.getEntityProperties(_this.entityID);
            var cardFront = CARD_IMAGE_BASE_URL + _this.getCard().suit + this.getCard().rank + ".jpg";
            print('card front is:' + cardFront);
            var frontVec = Quat.getUp(myProps.rotation);
            var forward = Vec3.sum(myProps.position, Vec3.multiply(0.0009, frontVec));
            _this.cardOverlay = Overlays.addOverlay("model", {
                url: CARD_MODEL_URL,
                position: forward,
                rotation: myProps.rotation,
                dimensions: myProps.dimensions,
                visible: true,
                drawInFront: true,
                parentID: myProps.id
            });
            print('jbp did attach card overlay:' + _this.cardOverlay)

            Script.setTimeout(function() {
                var success = Overlays.editOverlay(_this.cardOverlay, {
                    textures: {
                        "file1": cardFront,
                        "file2": CARD_BACK_IMAGE_URL,
                    },
                })
                print('jbp success on edit? ' + success)
            }, 1)


        },
        detachCardOverlay: function() {
            print('jbp should detach card overlay')

            Overlays.deleteOverlay(_this.cardOverlay);
        },
        getCard: function() {
            return _this.getCurrentUserData().playingCards.card;
        },
        setCurrentUserData: function(data) {
            var userData = getCurrentUserData();
            userData.playingCards = data;
            Entities.editEntity(_this.entityID, {
                userData: userData
            });
        },
        getCurrentUserData: function() {
            var props = Entities.getEntityProperties(_this.entityID);
            var json = null;
            try {
                json = JSON.parse(props.userData);
            } catch (e) {
                return;
            }
            return json;
        },
    }

    return new PlayingCard();
})