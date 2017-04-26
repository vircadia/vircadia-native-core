//
//  Created by Thijs Wenker on 3/31/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;

    var entityJSONCache = {};

    var SANITIZE_PROPERTIES = ['childEntities', 'parentID', 'id'];
    var ZERO_UUID = '{00000000-0000-0000-0000-000000000000}';

    function entityListToTree(entitiesList) {
        function entityListToTreeRecursive(properties) {
            properties.childEntities = [];
            entitiesList.forEach(function(entityProperties) {
                if (properties.id === entityProperties.parentID) {
                    properties.childEntities.push(entityListToTreeRecursive(entityProperties));
                }
            });
            return properties;
        }
        var entityTree = [];
        entitiesList.forEach(function(entityProperties) {
            if (entityProperties.parentID === undefined || entityProperties.parentID === ZERO_UUID) {
                entityTree.push(entityListToTreeRecursive(entityProperties));
            }
        });
        return entityTree;
    }

    function importEntitiesJSON(importLink, parentProperties, overrideProperties) {
        if (parentProperties === undefined) {
            parentProperties = {};
        }
        if (overrideProperties !== undefined) {
            parentProperties.overrideProperties = overrideProperties;
        }
        try {
            var entityJSONImport = Script.require(importLink).Entities;
            parentProperties.childEntities = entityListToTree(entityJSONImport);
            return parentProperties;
        } catch (e) {
            print('Failed importing entities JSON because: ' + JSON.stringify(e));
        }
        return null;
    }

    function fixLocalPath(link) {
        if (link.indexOf('://') === -1) {
            return Script.resolvePath(link);
        }
        return link;
    }

    // Creates an entity and returns a mixed object of the creation properties and the assigned entityID
    var createEntity = function(entityProperties, parent, overrideProperties) {
        // JSON.stringify -> JSON.parse trick to create a fresh copy of JSON data
        var newEntityProperties = JSON.parse(JSON.stringify(entityProperties));
        if (overrideProperties !== undefined) {
            Object.keys(overrideProperties).forEach(function(key) {
                newEntityProperties[key] = overrideProperties[key];
            });
        }
        if (parent.rotation !== undefined) {
            if (newEntityProperties.rotation !== undefined) {
                newEntityProperties.rotation = Quat.multiply(parent.rotation, newEntityProperties.rotation);
            } else {
                newEntityProperties.rotation = parent.rotation;
            }
        }
        if (parent.position !== undefined) {
            var localPosition = (parent.rotation !== undefined) ?
                Vec3.multiplyQbyV(parent.rotation, newEntityProperties.position) : newEntityProperties.position;
            newEntityProperties.position = Vec3.sum(localPosition, parent.position);
        }
        if (parent.id !== undefined) {
            newEntityProperties.parentID = parent.id;
        }
        // Fix up paths
        if (entityProperties.modelURL !== undefined) {
            newEntityProperties.modelURL = fixLocalPath(newEntityProperties.modelURL);
        }
        if (entityProperties.script !== undefined) {
            newEntityProperties.script = fixLocalPath(newEntityProperties.script);
        }
        newEntityProperties.id = Entities.addEntity(newEntityProperties);
        return newEntityProperties;
    };

    var createEntitiesFromTree = function(entityTree, parent, overrideProperties) {
        if (parent === undefined) {
            parent = {};
        }
        if (parent.overrideProperties !== undefined) {
            overrideProperties = parent.overrideProperties;
        }
        var createdTree = [];
        entityTree.forEach(function(entityProperties) {
            var sanitizedProperties = {};
            Object.keys(entityProperties).forEach(function(propertyKey) {
                if (!entityProperties.hasOwnProperty(propertyKey) || SANITIZE_PROPERTIES.indexOf(propertyKey) !== -1) {
                    return true;
                }
                sanitizedProperties[propertyKey] = entityProperties[propertyKey];
            });

            // Allow for non-entity parent objects, this allows us to offset groups of entities to a specific position/rotation
            var parentProperties = sanitizedProperties;
            if (entityProperties.type !== undefined) {
                parentProperties = createEntity(sanitizedProperties, parent, overrideProperties);
            }
            if (entityProperties.childEntities !== undefined) {
                parentProperties.childEntities =
                    createEntitiesFromTree(entityProperties.childEntities, parentProperties, overrideProperties);
            }
            createdTree.push(parentProperties);
        });
        return createdTree;
    };

    // listens for a release message from entities with the snap to grid script
    // checks for the nearest snap point and sends a message back to the entity
    function ImportGamePiece(url, spawnLocation, spawnRotation) {
        var fullURL = Script.resolvePath(url);
        print('CREATE PastedItem FROM SPAWNER: ' + fullURL);
        var _created = [];

        function create() {
            var entitiesTree;
            if (entityJSONCache[fullURL]) {
                entitiesTree = entityJSONCache[fullURL];
            } else {
                entitiesTree = importEntitiesJSON(fullURL);
                entityJSONCache[fullURL] = entitiesTree;
            }

            entitiesTree.position = spawnLocation;
            entitiesTree.rotation = spawnRotation;
            // print('entityTree: ' + JSON.stringify(entitiesTree));

            // var entities = importEntitiesJSON(fullURL);
            // var success = Clipboard.importEntities(fullURL);
            // var dimensions = Clipboard.getContentsDimensions();
            // we want the bottom of any piece to actually be on the board, so we add half of the height of the piece to the location when we paste it,
            // spawnLocation.y += (0.5 * dimensions.y);
            // if (success === true) {
                // created = Clipboard.pasteEntities(spawnLocation);
            //    this.created = created;
            //    print('created ' + created);
            // }
            _created = createEntitiesFromTree([entitiesTree]);
        }

        function cleanup() {
            _created.forEach(function(obj) {
                print('removing: ' + JSON.stringify(obj));
                Entities.deleteEntity(obj.id);
            });
        }

        create();

        this.cleanup = cleanup;

    }

    function Tile(rowIndex, columnIndex) {
        var side = _this.tableSideSize / _this.game.startingArrangement.length;
        var rightAmount = rowIndex * side;
        rightAmount += (0.5 * side);
        var forwardAmount = columnIndex * side;
        forwardAmount += (0.5 * side);
        var localPosition = {
            x: rightAmount - (_this.matDimensions.x * 0.5),
            y: 0,
            z: forwardAmount - (_this.matDimensions.y * 0.5)
        };
        this.startingPosition = _this.matCenter;

        this.middle = Vec3.sum(this.startingPosition, Vec3.multiplyQbyV(_this.tableRotation, localPosition));

        var splitURL = _this.game.startingArrangement[rowIndex][columnIndex].split(":");
        if (splitURL[0] === '1') {
            this.url = Script.resolvePath(_this.game.pieces[0][splitURL[1]]);
        }
        if (splitURL[0] === '2') {
            this.url = Script.resolvePath(_this.game.pieces[1][splitURL[1]]);
        }
        if (splitURL[0] === 'empty') {
            this.url = 'empty';
        }
    }

    function EntitySpawner() {
        _this = this;
    }

    EntitySpawner.prototype = {
        matDimensions: null,
        matCenter: null,
        matCorner: null,
        tableRotation: null,
        items: [],
        toCleanup: [],
        preload: function(id) {
            print('JBP preload entity spawner');
            _this.entityID = id;
        },
        createSingleEntity: function(url, spawnLocation, spawnRotation) {
            if (url === 'empty') {
                return null;
            }
            var item = new ImportGamePiece(url, spawnLocation, spawnRotation);
            _this.items.push(item);
            return item;
        },
        changeMatPicture: function(mat) {
            var fullURL = Script.resolvePath(_this.game.matURL);
            print('changing mat: ' + fullURL);
            Entities.editEntity(mat, {
                textures: JSON.stringify({
                    Picture: fullURL
                })
            });
        },
        spawnEntities: function(id, params) {
            this.items = [];
            var matEntity = params[1];
            var matProperties = Entities.getEntityProperties(matEntity, ['dimensions', 'position', 'rotation',
                'parentID']);
            var dimensions = Entities.getEntityProperties(matEntity, 'dimensions').dimensions;
            _this.game = JSON.parse(params[0]);
            _this.matDimensions = matProperties.dimensions;
            _this.matCenter = matProperties.position;
            _this.matCorner = {
                x: _this.matCenter.x - (dimensions.x * 0.5),
                y: _this.matCenter.y,
                z: _this.matCenter.z + (dimensions.y * 0.5)
            };
            _this.matRotation = matProperties.rotation;
            var tableID = matProperties.parentID;
            _this.tableRotation = Entities.getEntityProperties(tableID, 'rotation').rotation;
            _this.tableSideSize = dimensions.x;
            _this.changeMatPicture(matEntity);
            if (_this.game.spawnStyle === 'pile') {
                _this.spawnByPile();
            } else if (_this.game.spawnStyle === 'arranged') {
                _this.spawnByArranged();
            }
        },
        spawnByPile: function() {
            var position = Entities.getEntityProperties(_this.entityID, 'position').position;
            for (var i = 0; i < _this.game.howMany; i++) {
                var spawnLocation = {
                    x: position.x,
                    y: position.y - 0.25,
                    z: position.z
                };
                var url;
                if (_this.game.identicalPieces === false) {
                    url = Script.resolvePath(_this.game.pieces[i]);
                } else {
                    url = Script.resolvePath(_this.game.pieces[0]);
                }
                _this.createSingleEntity(url, spawnLocation, _this.tableRotation);
            }
        },
        spawnByArranged: function() {
            // make sure to set userData.gameTable.attachedTo appropriately
            _this.setupGrid();
        },
        createDebugEntity: function(position) {
            return Entities.addEntity({
                type: 'Sphere',
                position: {
                    x: position.x,
                    y: position.y + 0.1,
                    z: position.z
                },
                color: {
                    red: 0,
                    green: 0,
                    blue: 255
                },
                dimensions: {
                    x: 0.1,
                    y: 0.1,
                    z: 0.1
                },
                collisionless: true
            });
        },
        setupGrid: function() {
            _this.tiles = [];

            for (var i = 0; i < _this.game.startingArrangement.length; i++) {
                for (var j = 0; j < _this.game.startingArrangement[i].length; j++) {
                    // print('jbp there is a tile at:: ' + i + "::" + j)
                    var tile = new Tile(i, j);
                    var item = _this.createSingleEntity(tile.url, tile.middle, _this.tableRotation);
                    if (_this.game.hasOwnProperty('snapToGrid') && _this.game.snapToGrid === true) {
                        var anchor = _this.createAnchorEntityAtPoint(tile.middle);
                        if (item !== null) {
                            Entities.editEntity(item, {
                                userData: JSON.stringify({
                                    gameTable: {
                                        attachedTo: anchor
                                    }
                                })
                            });
                        }
                    }

                    _this.tiles.push(tile);
                }
            }
        },
        findMidpoint: function(start, end) {
            var xy = Vec3.sum(start, end);
            return Vec3.multiply(0.5, xy);
        },
        createAnchorEntityAtPoint: function(position) {
            var properties = {
                type: 'Zone',
                name:'Game Table Snap To Grid Anchor',
                description: 'hifi:gameTable:anchor',
                visible: false,
                collisionless: true,
                dimensions: {
                    x: 0.075,
                    y: 0.075,
                    z: 0.075
                },
                parentID: _this.entityID,
                position: position,
                userData: 'available'
            };
            return Entities.addEntity(properties);
        },
        setCurrentUserData: function(data) {
            var userData = _this.getCurrentUserData();
            userData.gameTableData = data;
            Entities.editEntity(_this.entityID, {
                userData: userData
            });
        },
        getCurrentUserData: function() {
            var userData = Entities.getEntityProperties(_this.entityID).userData;
            try {
                return JSON.parse(userData);
            } catch (e) {
                // e
            }
            return null;
        },
        cleanupEntitiesList: function() {
            _this.items.forEach(function(item) {
                item.cleanup();
            });
        },
        unload: function() {
            _this.toCleanup.forEach(function(item) {
                Entities.deleteEntity(item);
            });
        }
    };
    return new EntitySpawner();
});
