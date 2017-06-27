"use strict";
/* eslint-env commonjs */
/* eslint-disable comma-dangle */

module.exports = DopplegangerAttachments;

DopplegangerAttachments.version = '0.0.0';
DopplegangerAttachments.WANT_DEBUG = false;

var _modelHelper = require('./model-helper.js'),
    modelHelper = _modelHelper.modelHelper,
    ModelReadyWatcher = _modelHelper.ModelReadyWatcher,
    utils = require('./utils.js');

function log() {
    print('doppleganger-attachments | ' + [].slice.call(arguments).join(' '));
}

function debugPrint() {
    DopplegangerAttachments.WANT_DEBUG && log.apply(this, arguments);
}

function DopplegangerAttachments(doppleganger, options) {
    utils.assign(this, {
        _options: options,
        doppleganger: doppleganger,
        attachments: undefined,
        manualJointSync: true,
        attachmentsUpdated: utils.signal(function attachmentsUpdated(currentAttachments, previousAttachments){}),
    });
    this._initialize();
    debugPrint('DopplegangerAttachments...', JSON.stringify(options));
}
DopplegangerAttachments.prototype = {
    // "hash" the current attachments (so that changes can be detected)
    getAttachmentsHash: function() {
        return JSON.stringify(this.doppleganger.avatar.getAttachmentsVariant());
    },
    // create a pure Object copy of the current native attachments variant
    _cloneAttachmentsVariant: function() {
        return JSON.parse(JSON.stringify(this.doppleganger.avatar.getAttachmentsVariant()));
    },
    // fetch and resolve attachments to include jointIndex and other relevant $metadata
    _getResolvedAttachments: function() {
        var attachments = this._cloneAttachmentsVariant(),
            objectID = this.doppleganger.objectID;
        function toString() {
            return '[attachment #' + this.$index + ' ' + this.$path + ' @ ' + this.jointName + '{' + this.$jointIndex + '}]';
        }
        return attachments.map(function(attachment, i) {
            var jointIndex = modelHelper.getJointIndex(objectID, attachment.jointName),
                path = (attachment.modelUrl+'').split(/[?#]/)[0].split('/').slice(-3).join('/');
            return Object.defineProperties(attachment, {
                $hash: { value: JSON.stringify(attachment) },
                $index: { value: i },
                $jointIndex: { value: jointIndex },
                $path: { value: path },
                toString: { value: toString },
            });
        });
    },
    // compare before / after attachment sets to determine which ones need to be (re)created
    refreshAttachments: function() {
        var before = this.attachments || [],
            beforeIndex = before.reduce(function(out, att, index) {
                out[att.$hash] = index; return out;
            }, {});
        var after = this._getResolvedAttachments(),
            afterIndex = after.reduce(function(out, att, index) {
                out[att.$hash] = index; return out;
            }, {});

        Object.keys(beforeIndex).concat(Object.keys(afterIndex)).forEach(function(hash) {
            if (hash in beforeIndex && hash in afterIndex) {
                // print('reusing previous attachment', hash);
                after[afterIndex[hash]] = before[beforeIndex[hash]];
            } else if (!(hash in afterIndex)) {
                var attachment = before[beforeIndex[hash]];
                attachment.properties && attachment.properties.objectID &&
                    modelHelper.deleteObject(attachment.properties.objectID);
                delete attachment.properties;
            }
        });
        this.attachments = after;
        this._createAttachmentObjects();
        this.attachmentsUpdated(after, before);
    },
    _createAttachmentObjects: function() {
        try {
            var attachments = this.attachments,
                parentID = this.doppleganger.objectID,
                jointNames = this.doppleganger.jointNames,
                properties = modelHelper.getProperties(this.doppleganger.objectID);

            debugPrint('DopplegangerAttachments..._createAttachmentObjects', JSON.stringify({
                type: properties.type,
                attachments: attachments.length,
                parentID: parentID,
                jointNames: jointNames.join(' | '),
            },0,2));
            return attachments.map(utils.bind(this, function(attachment, i) {
                var type = modelHelper.type(attachment.properties && attachment.properties.objectID);
                if (type === 'overlay') {
                    debugPrint('skipping already-provisioned attachment object', type, attachment.properties && attachment.properties.name);
                    return attachment;
                }
                var jointIndex = attachment.$jointIndex, // jointNames.indexOf(attachment.jointName),
                    scale = this.doppleganger.avatar.scale * (attachment.scale||1.0);

                attachment.properties = utils.assign({
                    name: attachment.toString(),
                    type: properties.type,
                    modelURL: attachment.modelUrl,
                    scale: scale,
                    dimensions: modelHelper.type(parentID) === 'entity' ?
                        Vec3.multiply(attachment.scale||1.0, Vec3.ONE) : undefined,
                    visible: false,
                    collisionless: true,
                    dynamic: false,
                    shapeType: 'none',
                    lifetime: 60,
                    grabbable: true,
                }, !this.manualJointSync && {
                    parentID: parentID,
                    parentJointIndex: jointIndex,
                });
                var objectID = attachment.properties.objectID = modelHelper.addObject(attachment.properties);
                attachment._resource = ModelCache.prefetch(attachment.properties.modelURL);
                attachment._modelReadier = new ModelReadyWatcher( {
                    resource: attachment._resource,
                    objectID: objectID,
                });
                this.doppleganger.activeChanged.connect(attachment._modelReadier, '_stop');

                attachment._modelReadier.modelReady.connect(this, function(err, result) {
                    if (err) {
                        log('>>>>> modelReady ERROR <<<<<: ' + err, attachment.modelUrl);
                        modelHelper.deleteObject(objectID);
                        return objectID = null;
                    }
                    debugPrint('attachment model ('+modelHelper.type(result.objectID)+') is ready; # joints ==',
                        result.jointNames && result.jointNames.length, result.naturalDimensions, result.objectID);
                    var properties = modelHelper.getProperties(result.objectID),
                        naturalDimensions = attachment.properties.naturalDimensions = properties.naturalDimensions;
                    modelHelper.editObject(result.objectID, {
                        dimensions: naturalDimensions ? Vec3.multiply(attachment.scale, naturalDimensions) : undefined,
                    });
                    this.onJointsUpdated(parentID); // trigger once to initialize position/rotation
                    // give time for model overlay to "settle", then make it visible
                    Script.setTimeout(utils.bind(this, function() {
                        modelHelper.editObject(result.objectID, {
                            visible: true,
                        });
                        attachment._loaded = true;
                    }), 100);
                });
                return attachment;
            }));
        } catch (e) {
            log('_createAttachmentObjects ERROR:', e.stack || e, e.fileName, e.lineNumber);
        }
    },

    _removeAttachmentObjects: function() {
        if (this.attachments) {
            this.attachments.forEach(function(attachment) {
                if (attachment.properties) {
                    if (attachment.properties.objectID) {
                        modelHelper.deleteObject(attachment.properties.objectID);
                    }
                    delete attachment.properties.objectID;
                }
            });
            delete this.attachments;
        }
    },

    onJointsUpdated: function onJointsUpdated(objectID) {
        var jointOrientations = modelHelper.getJointOrientations(objectID),
            jointPositions = modelHelper.getJointPositions(objectID),
            parentID = objectID,
            avatarScale = this.doppleganger.scale,
            manualJointSync = this.manualJointSync;

        if (!this.attachments) {
            this.refreshAttachments();
        }
        var updatedObjects = this.attachments.reduce(function(updates, attachment, i) {
            if (!attachment.properties || !attachment._loaded) {
                return updates;
            }
            var objectID = attachment.properties.objectID,
                jointIndex = attachment.$jointIndex,
                jointOrientation = jointOrientations[jointIndex],
                jointPosition = jointPositions[jointIndex];

            var translation = Vec3.multiply(avatarScale, attachment.translation),
                rotation = Quat.fromVec3Degrees(attachment.rotation),
                localPosition = Vec3.multiplyQbyV(jointOrientation, translation),
                localRotation = rotation;

            updates[objectID] = manualJointSync ? {
                visible: true,
                position: Vec3.sum(jointPosition, localPosition),
                rotation: Quat.multiply(jointOrientation, localRotation),
                scale: avatarScale * attachment.scale,
            } : {
                visible: true,
                parentID: parentID,
                parentJointIndex: jointIndex,
                localRotation: localRotation,
                localPosition: localPosition,
                scale: attachment.scale,
            };
            onJointsUpdated[objectID] = updates[objectID];
            return updates;
        }, {});
        modelHelper.editObjects(updatedObjects);
    },

    _initialize: function() {
        var doppleganger = this.doppleganger;
        if ('$attachmentControls' in doppleganger) {
            throw new Error('only one set of debug controls can be added per doppleganger');
        }
        doppleganger.$attachmentControls = this;
        doppleganger.activeChanged.connect(this, function(active) {
            if (active) {
                doppleganger.jointsUpdated.connect(this, 'onJointsUpdated');
            } else {
                doppleganger.jointsUpdated.disconnect(this, 'onJointsUpdated');
                this._removeAttachmentObjects();
            }
        });

        Script.scriptEnding.connect(this, '_removeAttachmentObjects');
    },
};
