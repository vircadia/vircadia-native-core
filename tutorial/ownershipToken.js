if (!Function.prototype.bind) {
  Function.prototype.bind = function(oThis) {
    if (typeof this !== 'function') {
      // closest thing possible to the ECMAScript 5
      // internal IsCallable function
      throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
    }

    var aArgs   = Array.prototype.slice.call(arguments, 1),
        fToBind = this,
        fNOP    = function() {},
        fBound  = function() {
          return fToBind.apply(this instanceof fNOP
                 ? this
                 : oThis,
                 aArgs.concat(Array.prototype.slice.call(arguments)));
        };

    if (this.prototype) {
      // Function.prototype doesn't have a prototype property
      fNOP.prototype = this.prototype;
    }
    fBound.prototype = new fNOP();

    return fBound;
  };
}

function getOption(options, key, defaultValue) {
    if (options.hasOwnProperty(key)) {
        return options[key];
    }
    return defaultValue;
}

var TOKEN_NAME_PREFIX = "ownership_token-";

function getOwnershipTokenID(parentEntityID) {
    var childEntityIDs = Entities.getChildrenIDs(parentEntityID);
    var ownerID = null;
    var ownerName = '';
    for (var i = 0; i < childEntityIDs.length; ++i) {
        var childID = childEntityIDs[i];
        var properties = Entities.getEntityProperties(childID, ['name', 'userData', 'lifetime', 'age']);
        var childName = properties.name;
        if (childName.indexOf(TOKEN_NAME_PREFIX) == 0) {
            if (ownerID === null || childName < ownerName) {
                ownerID = childID;
                ownerName = childName;
            }
        }
    }
    return ownerID;
}

function createOwnershipToken(name, parentEntityID) {
    return Entities.addEntity({
        type: "Box",
        name: TOKEN_NAME_PREFIX + name,
        visible: false,
        parentID: parentEntityID,
        locationPosition: { x: 0, y: 0, z: 0 },
        dimensions: { x: 100, y: 100, z: 100 },
        collisionless: true,
        lifetime: 5
    });
}

var DEBUG = true;
function debug() {
    if (DEBUG) {
        var args = Array.prototype.slice.call(arguments);
        print.apply(this, args);
    }
}

var TOKEN_STATE_DESTROYED = -1;
var TOKEN_STATE_UNOWNED = 0;
var TOKEN_STATE_REQUESTING_OWNERSHIP = 1;
var TOKEN_STATE_OWNED = 2;

OwnershipToken = function(name, parentEntityID, options) {
    this.name = MyAvatar.sessionUUID + "-" + Math.floor(Math.random() * 10000000);
    this.name = Math.floor(Math.random() * 10000000);
    this.parentEntityID = parentEntityID;

    // How often to check whether the token is available if we don't currently own it
    this.checkEverySeconds = getOption(options, 'checkEverySeconds', 1000);
    this.updateTokenLifetimeEvery = getOption(options, 'updateTokenLifetimeEvery', 2000);

    this.onGainedOwnership = getOption(options, 'onGainedOwnership', function() { });
    this.onLostOwnership = getOption(options, 'onLostOwnership', function() { });

    this.ownershipTokenID = null;
    this.setState(TOKEN_STATE_UNOWNED);
};

OwnershipToken.prototype = {
    destroy: function() {
        debug(this.name, "Destroying token");
        this.setState(TOKEN_STATE_DESTROYED);
    },

    setState: function(newState) {
        if (this.state == newState) {
            debug(this.name, "Warning: Trying to set state to the current state");
            return;
        }

        if (this.updateLifetimeID) {
            Script.clearInterval(this.updateLifetimeID);
            this.updateLifetimeID = null;
        }

        if (this.checkOwnershipAvailableID) {
            Script.clearInterval(this.checkOwnershipAvailableID);
            this.checkOwnershipAvailableID = null;
        }

        if (this.state == TOKEN_STATE_OWNED) {
            this.onLostOwnership(this);
        }

        if (newState == TOKEN_STATE_UNOWNED) {
            this.checkOwnershipAvailableID = Script.setInterval(
                    this.tryRequestingOwnership.bind(this), this.checkEverySeconds);

        } else if (newState == TOKEN_STATE_REQUESTING_OWNERSHIP) {

        } else if (newState == TOKEN_STATE_OWNED) {
            this.onGainedOwnership(this);
            this.updateLifetimeID = Script.setInterval(
                    this.updateTokenLifetime.bind(this), this.updateTokenLifetimeEvery);
        } else if (newState == TOKEN_STATE_DESTROYED) {
            Entities.deleteEntity(this.ownershipTokenID);
        }

        debug(this.name, "Info: Switching to state:", newState);
        this.state = newState;
    },
    updateTokenLifetime: function() {
        if (this.state != TOKEN_STATE_OWNED) {
            debug(this.name, "Error: Trying to update token while it is unowned");
            return;
        }

        debug(this.name, "Updating entity lifetime");
        var age = Entities.getEntityProperties(this.ownershipTokenID, 'age').age;
        Entities.editEntity(this.ownershipTokenID, {
            lifetime: age + 5
        });
    },
    tryRequestingOwnership: function() {
        if (this.state == TOKEN_STATE_REQUESTING_OWNERSHIP || this.state == TOKEN_STATE_OWNED) {
            debug(this.name, "We already have or are requesting ownership");
            return;
        }

        var ownerID = getOwnershipTokenID(this.parentEntityID);
        if (ownerID !== null) {
            // Already owned, return
            debug(this.name, "Token already owned by another client, return");
            return;
        }

        this.ownershipTokenID = createOwnershipToken(this.name, this.parentEntityID);
        this.setState(TOKEN_STATE_REQUESTING_OWNERSHIP);

        function checkOwnershipRequest() {
            var ownerID = getOwnershipTokenID(this.parentEntityID);
            if (ownerID == this.ownershipTokenID) {
                debug(this.name, "Info: Obtained ownership");
                this.setState(TOKEN_STATE_OWNED);
            } else {
                if (ownerID === null) {
                    debug(this.name, "Warning: Checked ownership request and no tokens existed");
                }
                debug(this.name, "Info: Lost ownership request")
                this.ownershipTokenID = null;
                this.setState(TOKEN_STATE_UNOWNED);
            }
        }

        Script.setTimeout(checkOwnershipRequest.bind(this), 2000);
    },
};
