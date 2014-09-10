//
//  editEntities.js
//  examples
//
//  Created by Clément Brisset on 4/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script allows you to edit models either with the razor hydras or with your mouse
//
//  If using the hydras :
//  grab grab models with the triggers, you can then move the models around or scale them with both hands.
//  You can switch mode using the bumpers so that you can move models around more easily.
//
//  If using the mouse :
//  - left click lets you move the model in the plane facing you.
//    If pressing shift, it will move on the horizontal plane it's in.
//  - right click lets you rotate the model. z and x give access to more axes of rotation while shift provides finer control.
//  - left + right click lets you scale the model.
//  - you can press r while holding the model to reset its rotation
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("toolBars.js");

var windowDimensions = Controller.getViewportDimensions();
var toolIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";
var toolHeight = 50;
var toolWidth = 50;

var LASER_WIDTH = 4;
var LASER_COLOR = { red: 255, green: 0, blue: 0 };
var LASER_LENGTH_FACTOR = 500;

var MIN_ANGULAR_SIZE = 2;
var MAX_ANGULAR_SIZE = 45;

var LEFT = 0;
var RIGHT = 1;

var SPAWN_DISTANCE = 1;
var DEFAULT_DIMENSION = 0.20;

var modelURLs = [
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/birarda/birarda_head.fbx",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/pug.fbx",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/newInvader16x16-large-purple.svo",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/minotaur/mino_full.fbx",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Combat_tank_V01.FBX",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/orc.fbx",
        "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/slimer.fbx"
    ];

var jointList = MyAvatar.getJointNames();

var mode = 0;
var isActive = false;


if (typeof String.prototype.fileName !== "function") {
    String.prototype.fileName = function () {
        return this.replace(/^(.*[\/\\])*/, "");
    };
}

if (typeof String.prototype.fileBase !== "function") {
    String.prototype.fileBase = function () {
        var filename = this.fileName();
        return filename.slice(0, filename.indexOf("."));
    };
}

if (typeof String.prototype.fileType !== "function") {
    String.prototype.fileType = function () {
        return this.slice(this.lastIndexOf(".") + 1);
    };
}

if (typeof String.prototype.path !== "function") {
    String.prototype.path = function () {
        return this.replace(/[\\\/][^\\\/]*$/, "");
    };
}

if (typeof String.prototype.regExpEscape !== "function") {
    String.prototype.regExpEscape = function () {
        return this.replace(/([$\^.+*?|\\\/{}()\[\]])/g, '\\$1');
    };
}

if (typeof String.prototype.toArrayBuffer !== "function") {
    String.prototype.toArrayBuffer = function () {
        var length,
            buffer,
            view,
            charCode,
            charCodes,
            i;

        charCodes = [];

        length = this.length;
        for (i = 0; i < length; i += 1) {
            charCode = this.charCodeAt(i);
            if (charCode <= 255) {
                charCodes.push(charCode);
            } else {
                charCodes.push(charCode / 256);
                charCodes.push(charCode % 256);
            }
        }

        length = charCodes.length;
        buffer = new ArrayBuffer(length);
        view = new Uint8Array(buffer);
        for (i = 0; i < length; i += 1) {
            view[i] = charCodes[i];
        }

        return buffer;
    };
}

if (typeof DataView.prototype.indexOf !== "function") {
    DataView.prototype.indexOf = function (searchString, position) {
        var searchLength = searchString.length,
            byteArrayLength = this.byteLength,
            maxSearchIndex = byteArrayLength - searchLength,
            searchCharCodes = [],
            found,
            i,
            j;

        searchCharCodes[searchLength] = 0;
        for (j = 0; j < searchLength; j += 1) {
            searchCharCodes[j] = searchString.charCodeAt(j);
        }

        i = position;
        found = false;
        while (i < maxSearchIndex && !found) {
            j = 0;
            while (j < searchLength && this.getUint8(i + j) === searchCharCodes[j]) {
                j += 1;
            }
            found = (j === searchLength);
            i += 1;
        }

        return found ? i - 1 : -1;
    };
}

if (typeof DataView.prototype.string !== "function") {
    DataView.prototype.string = function (start, length) {
        var charCodes = [],
            end,
            i;

        if (start === undefined) {
            start = 0;
        }
        if (length === undefined) {
            length = this.length;
        }

        end = start + length;
        for (i = start; i < end; i += 1) {
            charCodes.push(this.getUint8(i));
        }

        return String.fromCharCode.apply(String, charCodes);
    };
}

var progressDialog = (function () {
    var that = {},
        progressBackground,
        progressMessage,
        cancelButton,
        displayed = false,
        backgroundWidth = 300,
        backgroundHeight = 100,
        messageHeight = 32,
        cancelWidth = 70,
        cancelHeight = 32,
        textColor = { red: 255, green: 255, blue: 255 },
        textBackground = { red: 52, green: 52, blue: 52 },
        backgroundUrl = toolIconUrl + "progress-background.svg",
        windowDimensions;

    progressBackground = Overlays.addOverlay("image", {
        width: backgroundWidth,
        height: backgroundHeight,
        imageURL: backgroundUrl,
        alpha: 0.9,
        visible: false
    });

    progressMessage = Overlays.addOverlay("text", {
        width: backgroundWidth - 40,
        height: messageHeight,
        text: "",
        textColor: textColor,
        backgroundColor: textBackground,
        alpha: 0.9,
        visible: false
    });

    cancelButton = Overlays.addOverlay("text", {
        width: cancelWidth,
        height: cancelHeight,
        text: "Cancel",
        textColor: textColor,
        backgroundColor: textBackground,
        alpha: 0.9,
        visible: false
    });

    function move() {
        var progressX,
            progressY;

        if (displayed) {

            if (windowDimensions.x === Window.innerWidth && windowDimensions.y === Window.innerHeight) {
                return;
            }
            windowDimensions.x = Window.innerWidth;
            windowDimensions.y = Window.innerHeight;

            progressX = (windowDimensions.x - backgroundWidth) / 2;  // Center.
            progressY = windowDimensions.y / 2 - backgroundHeight;  // A little up from center.

            Overlays.editOverlay(progressBackground, { x: progressX, y: progressY });
            Overlays.editOverlay(progressMessage, { x: progressX + 20, y: progressY + 15 });
            Overlays.editOverlay(cancelButton, {
                x: progressX + backgroundWidth - cancelWidth - 20,
                y: progressY + backgroundHeight - cancelHeight - 15
            });
        }
    }
    that.move = move;

    that.onCancel = undefined;

    function open(message) {
        if (!displayed) {
            windowDimensions = { x: 0, y : 0 };
            displayed = true;
            move();
            Overlays.editOverlay(progressBackground, { visible: true });
            Overlays.editOverlay(progressMessage, { visible: true, text: message });
            Overlays.editOverlay(cancelButton, { visible: true });
        } else {
            throw new Error("open() called on progressDialog when already open");
        }
    }
    that.open = open;

    function isOpen() {
        return displayed;
    }
    that.isOpen = isOpen;

    function update(message) {
        if (displayed) {
            Overlays.editOverlay(progressMessage, { text: message });
        } else {
            throw new Error("update() called on progressDialog when not open");
        }
    }
    that.update = update;

    function close() {
        if (displayed) {
            Overlays.editOverlay(cancelButton, { visible: false });
            Overlays.editOverlay(progressMessage, { visible: false });
            Overlays.editOverlay(progressBackground, { visible: false });
            displayed = false;
        } else {
            throw new Error("close() called on progressDialog when not open");
        }
    }
    that.close = close;

    function mousePressEvent(event) {
        if (Overlays.getOverlayAtPoint({ x: event.x, y: event.y }) === cancelButton) {
            if (typeof this.onCancel === "function") {
                close();
                this.onCancel();
            }
            return true;
        }
        return false;
    }
    that.mousePressEvent = mousePressEvent;

    function cleanup() {
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(progressMessage);
        Overlays.deleteOverlay(progressBackground);
    }
    that.cleanup = cleanup;

    return that;
}());

var httpMultiPart = (function () {
    var that = {},
        parts,
        byteLength,
        boundaryString,
        crlf;

    function clear() {
        boundaryString = "--boundary_" + String(Uuid.generate()).slice(1, 36) + "=";
        parts = [];
        byteLength = 0;
        crlf = "";
    }
    that.clear = clear;

    function boundary() {
        return boundaryString.slice(2);
    }
    that.boundary = boundary;

    function length() {
        return byteLength;
    }
    that.length = length;

    function add(object) {
        // - name, string
        // - name, buffer
        var buffer,
            string,
            stringBuffer,
            compressedBuffer;

        if (object.name === undefined) {

            throw new Error("Item to add to HttpMultiPart must have a name");

        } else if (object.string !== undefined) {
            //--<boundary>=
            //Content-Disposition: form-data; name="model_name"
            //
            //<string>

            string = crlf + boundaryString + "\r\n"
                + "Content-Disposition: form-data; name=\"" + object.name + "\"\r\n"
                + "\r\n"
                + object.string;
            buffer = string.toArrayBuffer();

        } else if (object.buffer !== undefined) {
            //--<boundary>=
            //Content-Disposition: form-data; name="fbx"; filename="<filename>"
            //Content-Type: application/octet-stream
            //
            //<buffer>

            string = crlf + boundaryString + "\r\n"
                + "Content-Disposition: form-data; name=\"" + object.name
                + "\"; filename=\"" + object.buffer.filename + "\"\r\n"
                + "Content-Type: application/octet-stream\r\n"
                + "\r\n";
            stringBuffer = string.toArrayBuffer();

            compressedBuffer = object.buffer.buffer.compress();
            buffer = new Uint8Array(stringBuffer.byteLength + compressedBuffer.byteLength);
            buffer.set(new Uint8Array(stringBuffer));
            buffer.set(new Uint8Array(compressedBuffer), stringBuffer.byteLength);

        } else {

            throw new Error("Item to add to HttpMultiPart not recognized");
        }

        byteLength += buffer.byteLength;
        parts.push(buffer);

        crlf = "\r\n";

        return true;
    }
    that.add = add;

    function response() {
        var buffer,
            index,
            str,
            i;

        str = crlf + boundaryString + "--\r\n";
        buffer = str.toArrayBuffer();
        byteLength += buffer.byteLength;
        parts.push(buffer);

        buffer = new Uint8Array(byteLength);
        index = 0;
        for (i = 0; i < parts.length; i += 1) {
            buffer.set(new Uint8Array(parts[i]), index);
            index += parts[i].byteLength;
        }

        return buffer;
    }
    that.response = response;

    clear();

    return that;
}());

var modelUploader = (function () {
    var that = {},
        modelFile,
        modelName,
        modelURL,
        modelCallback,
        isProcessing,
        fstBuffer,
        fbxBuffer,
        //svoBuffer,
        mapping,
        geometry,
        API_URL = "https://data.highfidelity.io/api/v1/models",
        MODEL_URL = "http://public.highfidelity.io/models/content",
        NAME_FIELD = "name",
        SCALE_FIELD = "scale",
        FILENAME_FIELD = "filename",
        TEXDIR_FIELD = "texdir",
        MAX_TEXTURE_SIZE = 1024;

    function info(message) {
        if (progressDialog.isOpen()) {
            progressDialog.update(message);
        } else {
            progressDialog.open(message);
        }
        print(message);
    }

    function error(message) {
        if (progressDialog.isOpen()) {
            progressDialog.close();
        }
        print(message);
        Window.alert(message);
    }

    function randomChar(length) {
        var characters = "0123457689abcdefghijklmnopqrstuvwxyz",
            string = "",
            i;

        for (i = 0; i < length; i += 1) {
            string += characters[Math.floor(Math.random() * 36)];
        }

        return string;
    }

    function resetDataObjects() {
        fstBuffer = null;
        fbxBuffer = null;
        //svoBuffer = null;
        mapping = {};
        geometry = {};
        geometry.textures = [];
        geometry.embedded = [];
    }

    function readFile(filename) {
        var url = "file:///" + filename,
            req = new XMLHttpRequest();

        req.open("GET", url, false);
        req.responseType = "arraybuffer";
        req.send();
        if (req.status !== 200) {
            error("Could not read file: " + filename + " : " + req.statusText);
            return null;
        }

        return {
            filename: filename.fileName(),
            buffer: req.response
        };
    }

    function readMapping(buffer) {
        var dv = new DataView(buffer.buffer),
            lines,
            line,
            tokens,
            i,
            name,
            value,
            remainder,
            existing;

        mapping = {};  // { name : value | name : { value : [remainder] } }
        lines = dv.string(0, dv.byteLength).split(/\r\n|\r|\n/);
        for (i = 0; i < lines.length; i += 1) {
            line = lines[i].trim();
            if (line.length > 0 && line[0] !== "#") {
                tokens = line.split(/\s*=\s*/);
                if (tokens.length > 1) {
                    name = tokens[0];
                    value = tokens[1];
                    if (tokens.length > 2) {
                        remainder = tokens.slice(2, tokens.length).join(" = ");
                    } else {
                        remainder = null;
                    }
                    if (tokens.length === 2 && mapping[name] === undefined) {
                        mapping[name] = value;
                    } else {
                        if (mapping[name] === undefined) {
                            mapping[name] = {};

                        } else if (typeof mapping[name] !== "object") {
                            existing = mapping[name];
                            mapping[name] = { existing : null };
                        }

                        if (mapping[name][value] === undefined) {
                            mapping[name][value] = [];
                        }
                        mapping[name][value].push(remainder);
                    }
                }
            }
        }
    }

    function writeMapping(buffer) {
        var name,
            value,
            remainder,
            i,
            string = "";

        for (name in mapping) {
            if (mapping.hasOwnProperty(name)) {
                if (typeof mapping[name] === "object") {
                    for (value in mapping[name]) {
                        if (mapping[name].hasOwnProperty(value)) {
                            remainder = mapping[name][value];
                            if (remainder === null) {
                                string += (name + " = " + value + "\n");
                            } else {
                                for (i = 0; i < remainder.length; i += 1) {
                                    string += (name + " = " + value + " = " + remainder[i] + "\n");
                                }
                            }
                        }
                    }
                } else {
                    string += (name + " = " + mapping[name] + "\n");
                }
            }
        }

        buffer.buffer = string.toArrayBuffer();
    }

    function readGeometry(fbxBuffer) {
        var textures,
            view,
            index,
            EOF,
            previousNodeFilename;

        // Reference:
        // http://code.blender.org/index.php/2013/08/fbx-binary-file-format-specification/

        textures = {};
        view = new DataView(fbxBuffer.buffer);
        EOF = false;

        function parseBinaryFBX() {
            var endOffset,
                numProperties,
                propertyListLength,
                nameLength,
                name,
                filename;

            endOffset = view.getUint32(index, true);
            numProperties = view.getUint32(index + 4, true);
            propertyListLength = view.getUint32(index + 8, true);
            nameLength = view.getUint8(index + 12);
            index += 13;

            if (endOffset === 0) {
                return;
            }
            if (endOffset < index || endOffset > view.byteLength) {
                EOF = true;
                return;
            }

            name = view.string(index, nameLength).toLowerCase();
            index += nameLength;

            if (name === "content" && previousNodeFilename !== "") {
                // Blender 2.71 exporter "embeds" external textures as empty binary blobs so ignore these
                if (propertyListLength > 5) {
                    geometry.embedded.push(previousNodeFilename);
                }
            }

            if (name === "relativefilename") {
                filename = view.string(index + 5, view.getUint32(index + 1, true)).fileName();
                if (!textures.hasOwnProperty(filename)) {
                    textures[filename] = "";
                    geometry.textures.push(filename);
                }
                previousNodeFilename = filename;
            } else {
                previousNodeFilename = "";
            }

            index += (propertyListLength);

            while (index < endOffset && !EOF) {
                parseBinaryFBX();
            }
        }

        function readTextFBX() {
            var line,
                view,
                viewLength,
                charCode,
                charCodes,
                numCharCodes,
                filename,
                relativeFilename = "",
                MAX_CHAR_CODES = 250;

            view = new Uint8Array(fbxBuffer.buffer);
            viewLength = view.byteLength;
            charCodes = [];
            numCharCodes = 0;

            for (index = 0; index < viewLength; index += 1) {
                charCode = view[index];
                if (charCode !== 9 && charCode !== 32) {
                    if (charCode === 10) {  // EOL. Can ignore EOF.
                        line = String.fromCharCode.apply(String, charCodes).toLowerCase();
                        // For embedded textures, "Content:" line immediately follows "RelativeFilename:" line.
                        if (line.slice(0, 8) === "content:" && relativeFilename !== "") {
                            geometry.embedded.push(relativeFilename);
                        }
                        if (line.slice(0, 17) === "relativefilename:") {
                            filename = line.slice(line.indexOf("\""), line.lastIndexOf("\"") - line.length).fileName();
                            if (!textures.hasOwnProperty(filename)) {
                                textures[filename] = "";
                                geometry.textures.push(filename);
                            }
                            relativeFilename = filename;
                        } else {
                            relativeFilename = "";
                        }
                        charCodes = [];
                        numCharCodes = 0;
                    } else {
                        if (numCharCodes < MAX_CHAR_CODES) {  // Only interested in start of line
                            charCodes.push(charCode);
                            numCharCodes += 1;
                        }
                    }
                }
            }
        }

        if (view.string(0, 18) === "Kaydara FBX Binary") {
            previousNodeFilename = "";

            index = 27;
            while (index < view.byteLength - 39 && !EOF) {
                parseBinaryFBX();
            }

        } else {

            readTextFBX();

        }
    }

    function readModel() {
        var fbxFilename,
            //svoFilename,
            fileType;

        info("Reading model file");
        print("Model file: " + modelFile);

        if (modelFile.toLowerCase().fileType() === "fst") {
            fstBuffer = readFile(modelFile);
            if (fstBuffer === null) {
                return false;
            }
            readMapping(fstBuffer);
            fileType = mapping[FILENAME_FIELD].toLowerCase().fileType();
            if (mapping.hasOwnProperty(FILENAME_FIELD)) {
                if (fileType === "fbx") {
                    fbxFilename = modelFile.path() + "\\" + mapping[FILENAME_FIELD];
                //} else if (fileType === "svo") {
                //    svoFilename = modelFile.path() + "\\" + mapping[FILENAME_FIELD];
                } else {
                    error("Unrecognized model type in FST file!");
                    return false;
                }
            } else {
                error("Model file name not found in FST file!");
                return false;
            }
        } else {
            fstBuffer = {
                filename: "Interface." + randomChar(6),  // Simulate avatar model uploading behaviour
                buffer: null
            };

            if (modelFile.toLowerCase().fileType() === "fbx") {
                fbxFilename = modelFile;
                mapping[FILENAME_FIELD] = modelFile.fileName();

            //} else if (modelFile.toLowerCase().fileType() === "svo") {
            //    svoFilename = modelFile;
            //    mapping[FILENAME_FIELD] = modelFile.fileName();

            } else {
                error("Unrecognized file type: " + modelFile);
                return false;
            }
        }

        if (!isProcessing) { return false; }

        if (fbxFilename) {
            fbxBuffer = readFile(fbxFilename);
            if (fbxBuffer === null) {
                return false;
            }

            if (!isProcessing) { return false; }

            readGeometry(fbxBuffer);
        }

        //if (svoFilename) {
        //    svoBuffer = readFile(svoFilename);
        //    if (svoBuffer === null) {
        //        return false;
        //    }
        //}

        // Add any missing basic mappings
        if (!mapping.hasOwnProperty(NAME_FIELD)) {
            mapping[NAME_FIELD] = modelFile.fileName().fileBase();
        }
        if (!mapping.hasOwnProperty(TEXDIR_FIELD)) {
            mapping[TEXDIR_FIELD] = ".";
        }
        if (!mapping.hasOwnProperty(SCALE_FIELD)) {
            mapping[SCALE_FIELD] = 1.0;
        }

        return true;
    }

    function setProperties() {
        var form = [],
            directory,
            displayAs,
            validateAs;

        progressDialog.close();
        print("Setting model properties");

        form.push({ label: "Name:", value: mapping[NAME_FIELD] });

        directory = modelFile.path() + "/" + mapping[TEXDIR_FIELD];
        displayAs = new RegExp("^" + modelFile.path().regExpEscape() + "[\\\\\\\/](.*)");
        validateAs = new RegExp("^" + modelFile.path().regExpEscape() + "([\\\\\\\/].*)?");

        form.push({
            label: "Texture directory:",
            directory: modelFile.path() + "/" + mapping[TEXDIR_FIELD],
            title: "Choose Texture Directory",
            displayAs: displayAs,
            validateAs: validateAs,
            errorMessage: "Texture directory must be subdirectory of the model directory."
        });

        form.push({ button: "Cancel" });

        if (!Window.form("Set Model Properties", form)) {
            print("User cancelled uploading model");
            return false;
        }

        mapping[NAME_FIELD] = form[0].value;
        mapping[TEXDIR_FIELD] = form[1].directory.slice(modelFile.path().length + 1);
        if (mapping[TEXDIR_FIELD] === "") {
            mapping[TEXDIR_FIELD] = ".";
        }

        writeMapping(fstBuffer);

        return true;
    }

    function createHttpMessage(callback) {
        var multiparts = [],
            lodCount,
            lodFile,
            lodBuffer,
            textureBuffer,
            textureSourceFormat,
            textureTargetFormat,
            embeddedTextures,
            i;

        info("Preparing to send model");

        // Model name
        if (mapping.hasOwnProperty(NAME_FIELD)) {
            multiparts.push({
                name : "model_name",
                string : mapping[NAME_FIELD]
            });
        } else {
            error("Model name is missing");
            httpMultiPart.clear();
            return;
        }

        // FST file
        if (fstBuffer) {
            multiparts.push({
                name : "fst",
                buffer: fstBuffer
            });
        }

        // FBX file
        if (fbxBuffer) {
            multiparts.push({
                name : "fbx",
                buffer: fbxBuffer
            });
        }

        // SVO file
        //if (svoBuffer) {
        //    multiparts.push({
        //        name : "svo",
        //        buffer: svoBuffer
        //    });
        //}

        // LOD files
        lodCount = 0;
        for (lodFile in mapping.lod) {
            if (mapping.lod.hasOwnProperty(lodFile)) {
                lodBuffer = readFile(modelFile.path() + "\/" + lodFile);
                if (lodBuffer === null) {
                    return;
                }
                multiparts.push({
                    name: "lod" + lodCount,
                    buffer: lodBuffer
                });
                lodCount += 1;
            }
            if (!isProcessing) { return; }
        }

        // Textures
        embeddedTextures = "|" + geometry.embedded.join("|") + "|";
        for (i = 0; i < geometry.textures.length; i += 1) {
            if (embeddedTextures.indexOf("|" + geometry.textures[i].fileName() + "|") === -1) {
                textureBuffer = readFile(modelFile.path() + "\/"
                    + (mapping[TEXDIR_FIELD] !== "." ? mapping[TEXDIR_FIELD] + "\/" : "")
                    + geometry.textures[i]);
                if (textureBuffer === null) {
                    return;
                }

                textureSourceFormat = geometry.textures[i].fileType().toLowerCase();
                textureTargetFormat = (textureSourceFormat === "jpg" ? "jpg" : "png");
                textureBuffer.buffer =
                    textureBuffer.buffer.recodeImage(textureSourceFormat, textureTargetFormat, MAX_TEXTURE_SIZE);
                textureBuffer.filename = textureBuffer.filename.slice(0, -textureSourceFormat.length) + textureTargetFormat;

                multiparts.push({
                    name: "texture" + i,
                    buffer: textureBuffer
                });
            }

            if (!isProcessing) { return; }
        }

        // Model category
        multiparts.push({
            name : "model_category",
            string : "content"
        });

        // Create HTTP message
        httpMultiPart.clear();
        Script.setTimeout(function addMultipart() {
            var multipart = multiparts.shift();
            httpMultiPart.add(multipart);

            if (!isProcessing) { return; }

            if (multiparts.length > 0) {
                Script.setTimeout(addMultipart, 25);
            } else {
                callback();
            }
        }, 25);
    }

    function sendToHighFidelity() {
        var req,
            uploadedChecks,
            HTTP_GET_TIMEOUT = 60,  // 1 minute
            HTTP_SEND_TIMEOUT = 900,  // 15 minutes
            UPLOADED_CHECKS = 30,
            CHECK_UPLOADED_TIMEOUT = 1,  // 1 second
            handleCheckUploadedResponses,
            handleUploadModelResponses,
            handleRequestUploadResponses;

        function uploadTimedOut() {
            error("Model upload failed: Internet request timed out!");
        }

        function debugResponse() {
            print("req.errorCode = " + req.errorCode);
            print("req.readyState = " + req.readyState);
            print("req.status = " + req.status);
            print("req.statusText = " + req.statusText);
            print("req.responseType = " + req.responseType);
            print("req.responseText = " + req.responseText);
            print("req.response = " + req.response);
            print("req.getAllResponseHeaders() = " + req.getAllResponseHeaders());
        }

        function checkUploaded() {
            if (!isProcessing) { return; }

            info("Checking uploaded model");

            req = new XMLHttpRequest();
            req.open("HEAD", modelURL, true);
            req.timeout = HTTP_GET_TIMEOUT * 1000;
            req.onreadystatechange = handleCheckUploadedResponses;
            req.ontimeout = uploadTimedOut;
            req.send();
        }

        handleCheckUploadedResponses = function () {
            //debugResponse();
            if (req.readyState === req.DONE) {
                if (req.status === 200) {
                    // Note: Unlike avatar models, for content models we don't need to refresh texture cache.
                    print("Model uploaded: " + modelURL);
                    progressDialog.close();
                    if (Window.confirm("Your model has been uploaded as: " + modelURL + "\nDo you want to rez it?")) {
                        modelCallback(modelURL);
                    }
                } else if (req.status === 404) {
                    if (uploadedChecks > 0) {
                        uploadedChecks -= 1;
                        Script.setTimeout(checkUploaded, CHECK_UPLOADED_TIMEOUT * 1000);
                    } else {
                        print("Error: " + req.status + " " + req.statusText);
                        error("We could not verify that your model was successfully uploaded but it may have been at: "
                            + modelURL);
                    }
                } else {
                    print("Error: " + req.status + " " + req.statusText);
                    error("There was a problem with your upload, please try again later.");
                }
            }
        };

        function uploadModel(method) {
            var url;

            if (!isProcessing) { return; }

            req = new XMLHttpRequest();
            if (method === "PUT") {
                url = API_URL + "\/" + modelName;
                req.open("PUT", url, true);  //print("PUT " + url);
            } else {
                url = API_URL;
                req.open("POST", url, true);  //print("POST " + url);
            }
            req.setRequestHeader("Content-Type", "multipart/form-data; boundary=\"" + httpMultiPart.boundary() + "\"");
            req.timeout = HTTP_SEND_TIMEOUT * 1000;
            req.onreadystatechange = handleUploadModelResponses;
            req.ontimeout = uploadTimedOut;
            req.send(httpMultiPart.response().buffer);
        }

        handleUploadModelResponses = function () {
            //debugResponse();
            if (req.readyState === req.DONE) {
                if (req.status === 200) {
                    uploadedChecks = UPLOADED_CHECKS;
                    checkUploaded();
                } else {
                    print("Error: " + req.status + " " + req.statusText);
                    error("There was a problem with your upload, please try again later.");
                }
            }
        };

        function requestUpload() {
            var url;

            if (!isProcessing) { return; }

            url = API_URL + "\/" + modelName;  // XMLHttpRequest automatically handles authorization of API requests.
            req = new XMLHttpRequest();
            req.open("GET", url, true);  //print("GET " + url);
            req.responseType = "json";
            req.timeout = HTTP_GET_TIMEOUT * 1000;
            req.onreadystatechange = handleRequestUploadResponses;
            req.ontimeout = uploadTimedOut;
            req.send();
        }

        handleRequestUploadResponses = function () {
            var response;

            //debugResponse();
            if (req.readyState === req.DONE) {
                if (req.status === 200) {
                    if (req.responseType === "json") {
                        response = JSON.parse(req.responseText);
                        if (response.status === "success") {
                            if (response.exists === false) {
                                uploadModel("POST");
                            } else if (response.can_update === true) {
                                uploadModel("PUT");
                            } else {
                                error("This model file already exists and is owned by someone else!");
                            }
                            return;
                        }
                    }
                } else {
                    print("Error: " + req.status + " " + req.statusText);
                }
                error("Model upload failed! Something went wrong at the data server.");
            }
        };

        info("Sending model to High Fidelity");

        requestUpload();
    }

    that.upload = function (file, callback) {

        modelFile = file;
        modelCallback = callback;

        isProcessing = true;

        progressDialog.onCancel = function () {
            print("User cancelled uploading model");
            isProcessing = false;
        };

        resetDataObjects();

        if (readModel()) {
            if (setProperties()) {
                modelName = mapping[NAME_FIELD];
                modelURL = MODEL_URL + "\/" + mapping[NAME_FIELD] + ".fst";  // All models are uploaded as an FST

                createHttpMessage(sendToHighFidelity);
            }
        }

        resetDataObjects();
    };

    return that;
}());

var toolBar = (function () {
    var that = {},
        toolBar,
        activeButton,
        newModelButton,
        newCubeButton,
        newSphereButton,
        browseModelsButton,
        loadURLMenuItem,
        loadFileMenuItem,
        menuItemWidth = 125,
        menuItemOffset,
        menuItemHeight,
        menuItemMargin = 5,
        menuTextColor = { red: 255, green: 255, blue: 255 },
        menuBackgoundColor = { red: 18, green: 66, blue: 66 };

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL);

        activeButton = toolBar.addTool({
            imageURL: toolIconUrl + "models-tool.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        }, true, false);

        newModelButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-model-tool.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        }, true, false);

        browseModelsButton = toolBar.addTool({
            imageURL: toolIconUrl + "list-icon.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

        menuItemOffset = toolBar.height / 3 + 2;
        menuItemHeight = Tool.IMAGE_HEIGHT / 2 - 2;

        loadURLMenuItem = Overlays.addOverlay("text", {
            x: newModelButton.x - menuItemWidth,
            y: newModelButton.y + menuItemOffset,
            width: menuItemWidth,
            height: menuItemHeight,
            backgroundColor: menuBackgoundColor,
            topMargin: menuItemMargin,
            text: "Model URL",
            alpha: 0.9,
            visible: false
        });

        loadFileMenuItem = Overlays.addOverlay("text", {
            x: newModelButton.x - menuItemWidth,
            y: newModelButton.y + menuItemOffset + menuItemHeight,
            width: menuItemWidth,
            height: menuItemHeight,
            backgroundColor: menuBackgoundColor,
            topMargin: menuItemMargin,
            text: "Model File",
            alpha: 0.9,
            visible: false
        });

        newCubeButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-cube.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

        newSphereButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-sphere.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

    }

    function toggleNewModelButton(active) {
        if (active === undefined) {
            active = !toolBar.toolSelected(newModelButton);
        }
        toolBar.selectTool(newModelButton, active);

        Overlays.editOverlay(loadURLMenuItem, { visible: active });
        Overlays.editOverlay(loadFileMenuItem, { visible: active });
    }

    function addModel(url) {
        var position;

        position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

        if (position.x > 0 && position.y > 0 && position.z > 0) {
            Entities.addEntity({
                type: "Model",
                position: position,
                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                modelURL: url
            });
            print("Model added: " + url);
        } else {
            print("Can't add model: Model would be out of bounds.");
        }
    }

    that.move = function () {
        var newViewPort,
            toolsX,
            toolsY;

        newViewPort = Controller.getViewportDimensions();

        if (toolBar === undefined) {
            initialize();

        } else if (windowDimensions.x === newViewPort.x &&
                   windowDimensions.y === newViewPort.y) {
            return;
        }

        windowDimensions = newViewPort;
        toolsX = windowDimensions.x - 8 - toolBar.width;
        toolsY = (windowDimensions.y - toolBar.height) / 2;

        toolBar.move(toolsX, toolsY);

        Overlays.editOverlay(loadURLMenuItem, { x: toolsX - menuItemWidth, y: toolsY + menuItemOffset });
        Overlays.editOverlay(loadFileMenuItem, { x: toolsX - menuItemWidth, y: toolsY + menuItemOffset + menuItemHeight });
    };

    that.mousePressEvent = function (event) {
        var clickedOverlay,
            url,
            file;

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (activeButton === toolBar.clicked(clickedOverlay)) {
            isActive = !isActive;
            return true;
        }

        if (newModelButton === toolBar.clicked(clickedOverlay)) {
            toggleNewModelButton();
            return true;
        }

        if (clickedOverlay === loadURLMenuItem) {
            toggleNewModelButton(false);
            url = Window.prompt("Model URL", modelURLs[Math.floor(Math.random() * modelURLs.length)]);
            if (url !== null && url !== "") {
                addModel(url);
            }
            return true;
        }

        if (clickedOverlay === loadFileMenuItem) {
            toggleNewModelButton(false);

            // TODO BUG: this is bug, if the user has never uploaded a model, this will throw an JS exception
            file = Window.browse("Select your model file ...",
                Settings.getValue("LastModelUploadLocation").path(), 
                "Model files (*.fst *.fbx)");
                //"Model files (*.fst *.fbx *.svo)");
            if (file !== null) {
                Settings.setValue("LastModelUploadLocation", file);
                modelUploader.upload(file, addModel);
            }
            return true;
        }

        if (browseModelsButton === toolBar.clicked(clickedOverlay)) {
            toggleNewModelButton(false);
            url = Window.s3Browse(".*(fbx|FBX)");
            if (url !== null && url !== "") {
                addModel(url);
            }
            return true;
        }

        if (newCubeButton === toolBar.clicked(clickedOverlay)) {
            var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

            if (position.x > 0 && position.y > 0 && position.z > 0) {
                Entities.addEntity({ 
                                type: "Box",
                                position: position,
                                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                                color: { red: 255, green: 0, blue: 0 }

                                });
            } else {
                print("Can't create box: Box would be out of bounds.");
            }
            return true;
        }

        if (newSphereButton === toolBar.clicked(clickedOverlay)) {
            var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

            if (position.x > 0 && position.y > 0 && position.z > 0) {
                Entities.addEntity({ 
                                type: "Sphere",
                                position: position,
                                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                                color: { red: 255, green: 0, blue: 0 }
                                });
            } else {
                print("Can't create box: Box would be out of bounds.");
            }
            return true;
        }


        return false;
    };

    that.cleanup = function () {
        toolBar.cleanup();
        Overlays.deleteOverlay(loadURLMenuItem);
        Overlays.deleteOverlay(loadFileMenuItem);
    };

    return that;
}());


var exportMenu = null;

var ExportMenu = function (opts) {
    var self = this;

    var windowDimensions = Controller.getViewportDimensions();
    var pos = { x: windowDimensions.x / 2, y: windowDimensions.y - 100 };

    this._onClose = opts.onClose || function () { };
    this._position = { x: 0.0, y: 0.0, z: 0.0 };
    this._scale = 1.0;

    var minScale = 1;
    var maxScale = 32768;
    var titleWidth = 120;
    var locationWidth = 100;
    var scaleWidth = 144;
    var exportWidth = 100;
    var cancelWidth = 100;
    var margin = 4;
    var height = 30;
    var outerHeight = height + (2 * margin);
    var buttonColor = { red: 128, green: 128, blue: 128 };

    var SCALE_MINUS = scaleWidth * 40.0 / 100.0;
    var SCALE_PLUS = scaleWidth * 63.0 / 100.0;

    var fullWidth = locationWidth + scaleWidth + exportWidth + cancelWidth + (2 * margin);
    var offset = fullWidth / 2;
    pos.x -= offset;

    var background = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y,
        opacity: 1,
        width: fullWidth,
        height: outerHeight,
        backgroundColor: { red: 200, green: 200, blue: 200 },
        text: "",
    });

    var titleText = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y - height,
        font: { size: 14 },
        width: titleWidth,
        height: height,
        backgroundColor: { red: 255, green: 255, blue: 255 },
        color: { red: 255, green: 255, blue: 255 },
        text: "Export Models"
    });

    var locationButton = Overlays.addOverlay("text", {
        x: pos.x + margin,
        y: pos.y + margin,
        width: locationWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        text: "0, 0, 0",
    });
    var scaleOverlay = Overlays.addOverlay("image", {
        x: pos.x + margin + locationWidth,
        y: pos.y + margin,
        width: scaleWidth,
        height: height,
        subImage: { x: 0, y: 3, width: 144, height: height },
        imageURL: toolIconUrl + "voxel-size-selector.svg",
        alpha: 0.9,
    });
    var scaleViewWidth = 40;
    var scaleView = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + SCALE_MINUS,
        y: pos.y + margin,
        width: scaleViewWidth,
        height: height,
        alpha: 0.0,
        color: { red: 255, green: 255, blue: 255 },
        text: "1"
    });
    var exportButton = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + scaleWidth,
        y: pos.y + margin,
        width: exportWidth,
        height: height,
        color: { red: 0, green: 255, blue: 255 },
        text: "Export"
    });
    var cancelButton = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + scaleWidth + exportWidth,
        y: pos.y + margin,
        width: cancelWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        text: "Cancel"
    });

    var voxelPreview = Overlays.addOverlay("cube", {
        position: { x: 0, y: 0, z: 0 },
        size: this._scale,
        color: { red: 255, green: 255, blue: 0 },
        alpha: 1,
        solid: false,
        visible: true,
        lineWidth: 4
    });

    this.parsePosition = function (str) {
        var parts = str.split(',');
        if (parts.length == 3) {
            var x = parseFloat(parts[0]);
            var y = parseFloat(parts[1]);
            var z = parseFloat(parts[2]);
            if (isFinite(x) && isFinite(y) && isFinite(z)) {
                return { x: x, y: y, z: z };
            }
        }
        return null;
    };

    this.showPositionPrompt = function () {
        var positionStr = self._position.x + ", " + self._position.y + ", " + self._position.z;
        while (1) {
            positionStr = Window.prompt("Position to export form:", positionStr);
            if (positionStr == null) {
                break;
            }
            var position = self.parsePosition(positionStr);
            if (position != null) {
                self.setPosition(position.x, position.y, position.z);
                break;
            }
            Window.alert("The position you entered was invalid.");
        }
    };

    this.setScale = function (scale) {
        self._scale = Math.min(maxScale, Math.max(minScale, scale));
        Overlays.editOverlay(scaleView, { text: self._scale });
        Overlays.editOverlay(voxelPreview, { size: self._scale });
    }

    this.decreaseScale = function () {
        self.setScale(self._scale /= 2);
    }

    this.increaseScale = function () {
        self.setScale(self._scale *= 2);
    }

    this.exportEntities = function() {
        var x = self._position.x;
        var y = self._position.y;
        var z = self._position.z;
        var s = self._scale;
        var filename = "models__" + Window.location.hostname + "__" + x + "_" + y + "_" + z + "_" + s + "__.svo";
        filename = Window.save("Select where to save", filename, "*.svo")
        if (filename) {
            var success = Clipboard.exportEntities(filename, x, y, z, s);
            if (!success) {
                Window.alert("Export failed: no models found in selected area.");
            }
        }
        self.close();
    };

    this.getPosition = function () {
        return self._position;
    };

    this.setPosition = function (x, y, z) {
        self._position = { x: x, y: y, z: z };
        var positionStr = x + ", " + y + ", " + z;
        Overlays.editOverlay(locationButton, { text: positionStr });
        Overlays.editOverlay(voxelPreview, { position: self._position });

    };

    this.mouseReleaseEvent = function (event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (clickedOverlay == locationButton) {
            self.showPositionPrompt();
        } else if (clickedOverlay == exportButton) {
            self.exportEntities();
        } else if (clickedOverlay == cancelButton) {
            self.close();
        } else if (clickedOverlay == scaleOverlay) {
            var x = event.x - pos.x - margin - locationWidth;
            print(x);
            if (x < SCALE_MINUS) {
                self.decreaseScale();
            } else if (x > SCALE_PLUS) {
                self.increaseScale();
            }
        }
    };

    this.close = function () {
        this.cleanup();
        this._onClose();
    };

    this.cleanup = function () {
        Overlays.deleteOverlay(background);
        Overlays.deleteOverlay(titleText);
        Overlays.deleteOverlay(locationButton);
        Overlays.deleteOverlay(exportButton);
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(voxelPreview);
        Overlays.deleteOverlay(scaleOverlay);
        Overlays.deleteOverlay(scaleView);
    };

    print("CONNECTING!");
    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
};

var ModelImporter = function (opts) {
    var self = this;

    var height = 30;
    var margin = 4;
    var outerHeight = height + (2 * margin);
    var titleWidth = 120;
    var cancelWidth = 100;
    var fullWidth = titleWidth + cancelWidth + (2 * margin);

    var localModels = Overlays.addOverlay("localmodels", {
        position: { x: 1, y: 1, z: 1 },
        scale: 1,
        visible: false
    });
    var importScale = 1;
    var importBoundaries = Overlays.addOverlay("cube", {
        position: { x: 0, y: 0, z: 0 },
        size: 1,
        color: { red: 128, blue: 128, green: 128 },
        lineWidth: 4,
        solid: false,
        visible: false
    });

    var pos = { x: windowDimensions.x / 2 - (fullWidth / 2), y: windowDimensions.y - 100 };

    var background = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y,
        opacity: 1,
        width: fullWidth,
        height: outerHeight,
        backgroundColor: { red: 200, green: 200, blue: 200 },
        visible: false,
        text: "",
    });

    var titleText = Overlays.addOverlay("text", {
        x: pos.x + margin,
        y: pos.y + margin,
        font: { size: 14 },
        width: titleWidth,
        height: height,
        backgroundColor: { red: 255, green: 255, blue: 255 },
        color: { red: 255, green: 255, blue: 255 },
        visible: false,
        text: "Import Models"
    });
    var cancelButton = Overlays.addOverlay("text", {
        x: pos.x + margin + titleWidth,
        y: pos.y + margin,
        width: cancelWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        visible: false,
        text: "Close"
    });
    this._importing = false;

    this.setImportVisible = function (visible) {
        Overlays.editOverlay(importBoundaries, { visible: visible });
        Overlays.editOverlay(localModels, { visible: visible });
        Overlays.editOverlay(cancelButton, { visible: visible });
        Overlays.editOverlay(titleText, { visible: visible });
        Overlays.editOverlay(background, { visible: visible });
    };

    var importPosition = { x: 0, y: 0, z: 0 };
    this.moveImport = function (position) {
        importPosition = position;
        Overlays.editOverlay(localModels, {
            position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
        });
        Overlays.editOverlay(importBoundaries, {
            position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
        });
    }

    this.mouseMoveEvent = function (event) {
        if (self._importing) {
            var pickRay = Camera.computePickRay(event.x, event.y);
            var intersection = Voxels.findRayIntersection(pickRay);

            var distance = 2;// * self._scale;

            if (false) {//intersection.intersects) {
                var intersectionDistance = Vec3.length(Vec3.subtract(pickRay.origin, intersection.intersection));
                if (intersectionDistance < distance) {
                    distance = intersectionDistance * 0.99;
                }

            }

            var targetPosition = {
                x: pickRay.origin.x + (pickRay.direction.x * distance),
                y: pickRay.origin.y + (pickRay.direction.y * distance),
                z: pickRay.origin.z + (pickRay.direction.z * distance)
            };

            if (targetPosition.x < 0) targetPosition.x = 0;
            if (targetPosition.y < 0) targetPosition.y = 0;
            if (targetPosition.z < 0) targetPosition.z = 0;

            var nudgeFactor = 1;
            var newPosition = {
                x: Math.floor(targetPosition.x / nudgeFactor) * nudgeFactor,
                y: Math.floor(targetPosition.y / nudgeFactor) * nudgeFactor,
                z: Math.floor(targetPosition.z / nudgeFactor) * nudgeFactor
            }

            self.moveImport(newPosition);
        }
    }

    this.mouseReleaseEvent = function (event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (clickedOverlay == cancelButton) {
            self._importing = false;
            self.setImportVisible(false);
        }
    };

    // Would prefer to use {4} for the coords, but it would only capture the last digit.
    var fileRegex = /__(.+)__(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)__/;
    this.doImport = function () {
        if (!self._importing) {
            var filename = Window.browse("Select models to import", "", "*.svo")
            if (filename) {
                parts = fileRegex.exec(filename);
                if (parts == null) {
                    Window.alert("The file you selected does not contain source domain or location information");
                } else {
                    var hostname = parts[1];
                    var x = parts[2];
                    var y = parts[3];
                    var z = parts[4];
                    var s = parts[5];
                    importScale = s;
                    if (hostname != location.hostname) {
                        if (!Window.confirm(("These models were not originally exported from this domain. Continue?"))) {
                            return;
                        }
                    } else {
                        if (Window.confirm(("Would you like to import back to the source location?"))) {
                            var success = Clipboard.importEntities(filename);
                            if (success) {
                                Clipboard.pasteEntities(x, y, z, 1);
                            } else {
                                Window.alert("There was an error importing the entity file.");
                            }
                            return;
                        }
                    }
                }
                var success = Clipboard.importEntities(filename);
                if (success) {
                    self._importing = true;
                    self.setImportVisible(true);
                    Overlays.editOverlay(importBoundaries, { size: s });
                } else {
                    Window.alert("There was an error importing the entity file.");
                }
            }
        }
    }

    this.paste = function () {
        if (self._importing) {
            // self._importing = false;
            // self.setImportVisible(false);
            Clipboard.pasteEntities(importPosition.x, importPosition.y, importPosition.z, 1);
        }
    }

    this.cleanup = function () {
        Overlays.deleteOverlay(localModels);
        Overlays.deleteOverlay(importBoundaries);
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(titleText);
        Overlays.deleteOverlay(background);
    }

    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
    Controller.mouseMoveEvent.connect(this.mouseMoveEvent);
};

var modelImporter = new ModelImporter();


function isLocked(properties) {
    // special case to lock the ground plane model in hq.
    if (location.hostname == "hq.highfidelity.io" &&
        properties.modelURL == "https://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/Terrain_Reduce_forAlpha.fbx") {
        return true;
    }
    return false;
}


function controller(wichSide) {
    this.side = wichSide;
    this.palm = 2 * wichSide;
    this.tip = 2 * wichSide + 1;
    this.trigger = wichSide;
    this.bumper = 6 * wichSide + 5;

    this.oldPalmPosition = Controller.getSpatialControlPosition(this.palm);
    this.palmPosition = Controller.getSpatialControlPosition(this.palm);

    this.oldTipPosition = Controller.getSpatialControlPosition(this.tip);
    this.tipPosition = Controller.getSpatialControlPosition(this.tip);

    this.oldUp = Controller.getSpatialControlNormal(this.palm);
    this.up = this.oldUp;

    this.oldFront = Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition));
    this.front = this.oldFront;

    this.oldRight = Vec3.cross(this.front, this.up);
    this.right = this.oldRight;

    this.oldRotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(this.palm));
    this.rotation = this.oldRotation;

    this.triggerValue = Controller.getTriggerValue(this.trigger);
    this.bumperValue = Controller.isButtonPressed(this.bumper);

    this.pressed = false; // is trigger pressed
    this.pressing = false; // is trigger being pressed (is pressed now but wasn't previously)

    this.grabbing = false;
    this.entityID = { isKnownID: false };
    this.modelURL = "";
    this.oldModelRotation;
    this.oldModelPosition;
    this.oldModelHalfDiagonal;

    this.positionAtGrab;
    this.rotationAtGrab;
    this.modelPositionAtGrab;
    this.rotationAtGrab;
    this.jointsIntersectingFromStart = [];

    this.laser = Overlays.addOverlay("line3d", {
        position: { x: 0, y: 0, z: 0 },
        end: { x: 0, y: 0, z: 0 },
        color: LASER_COLOR,
        alpha: 1,
        visible: false,
        lineWidth: LASER_WIDTH,
        anchor: "MyAvatar"
    });

    this.guideScale = 0.02;
    this.ball = Overlays.addOverlay("sphere", {
        position: { x: 0, y: 0, z: 0 },
        size: this.guideScale,
        solid: true,
        color: { red: 0, green: 255, blue: 0 },
        alpha: 1,
        visible: false,
        anchor: "MyAvatar"
    });
    this.leftRight = Overlays.addOverlay("line3d", {
        position: { x: 0, y: 0, z: 0 },
        end: { x: 0, y: 0, z: 0 },
        color: { red: 0, green: 0, blue: 255 },
        alpha: 1,
        visible: false,
        lineWidth: LASER_WIDTH,
        anchor: "MyAvatar"
    });
    this.topDown = Overlays.addOverlay("line3d", {
                                       position: { x: 0, y: 0, z: 0 },
                                       end: { x: 0, y: 0, z: 0 },
                                       color: { red: 0, green: 0, blue: 255 },
                                       alpha: 1,
                                       visible: false,
                                       lineWidth: LASER_WIDTH,
                                       anchor: "MyAvatar"
                                       });
    

    
    this.grab = function (entityID, properties) {
        if (isLocked(properties)) {
            print("Model locked " + entityID.id);
        } else {
            print("Grabbing " + entityID.id);
            this.grabbing = true;
            this.entityID = entityID;
            this.modelURL = properties.modelURL;

            this.oldModelPosition = properties.position;
            this.oldModelRotation = properties.rotation;
            this.oldModelHalfDiagonal = Vec3.length(properties.dimensions) / 2.0;

            this.positionAtGrab = this.palmPosition;
            this.rotationAtGrab = this.rotation;
            this.modelPositionAtGrab = properties.position;
            this.rotationAtGrab = properties.rotation;
            this.jointsIntersectingFromStart = [];
            for (var i = 0; i < jointList.length; i++) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(jointList[i]), this.oldModelPosition);
                if (distance < this.oldModelHalfDiagonal) {
                    this.jointsIntersectingFromStart.push(i);
                }
            }
            this.showLaser(false);
        }
    }

    this.release = function () {
        if (this.grabbing) {
            jointList = MyAvatar.getJointNames();

            var closestJointIndex = -1;
            var closestJointDistance = 10;
            for (var i = 0; i < jointList.length; i++) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(jointList[i]), this.oldModelPosition);
                if (distance < closestJointDistance) {
                    closestJointDistance = distance;
                    closestJointIndex = i;
                }
            }

            if (closestJointIndex != -1) {
                print("closestJoint: " + jointList[closestJointIndex]);
                print("closestJointDistance (attach max distance): " + closestJointDistance + " (" + this.oldModelHalfDiagonal + ")");
            }

            if (closestJointDistance < this.oldModelHalfDiagonal) {

                if (this.jointsIntersectingFromStart.indexOf(closestJointIndex) != -1 ||
                    (leftController.grabbing && rightController.grabbing &&
                    leftController.entityID.id == rightController.entityID.id)) {
                    // Do nothing
                } else {
                    print("Attaching to " + jointList[closestJointIndex]);
                    var jointPosition = MyAvatar.getJointPosition(jointList[closestJointIndex]);
                    var jointRotation = MyAvatar.getJointCombinedRotation(jointList[closestJointIndex]);

                    var attachmentOffset = Vec3.subtract(this.oldModelPosition, jointPosition);
                    attachmentOffset = Vec3.multiplyQbyV(Quat.inverse(jointRotation), attachmentOffset);
                    var attachmentRotation = Quat.multiply(Quat.inverse(jointRotation), this.oldModelRotation);

                    MyAvatar.attach(this.modelURL, jointList[closestJointIndex],
                                    attachmentOffset, attachmentRotation, 2.0 * this.oldModelHalfDiagonal,
                                    true, false);
                    Entities.deleteEntity(this.entityID);
                }
            }
        }

        this.grabbing = false;
        this.entityID.isKnownID = false;
        this.jointsIntersectingFromStart = [];
        this.showLaser(true);
    }

    this.checkTrigger = function () {
        if (this.triggerValue > 0.9) {
            if (this.pressed) {
                this.pressing = false;
            } else {
                this.pressing = true;
            }
            this.pressed = true;
        } else {
            this.pressing = false;
            this.pressed = false;
        }
    }

    this.checkEntity = function (properties) {
        // special case to lock the ground plane model in hq.
        if (isLocked(properties)) {
            return { valid: false };
        }


        //                P         P - Model
        //               /|         A - Palm
        //              / | d       B - unit vector toward tip
        //             /  |         X - base of the perpendicular line
        //            A---X----->B  d - distance fom axis
        //              x           x - distance from A
        //
        //            |X-A| = (P-A).B
        //            X == A + ((P-A).B)B
        //            d = |P-X|

        var A = this.palmPosition;
        var B = this.front;
        var P = properties.position;

        var x = Vec3.dot(Vec3.subtract(P, A), B);
        var y = Vec3.dot(Vec3.subtract(P, A), this.up);
        var z = Vec3.dot(Vec3.subtract(P, A), this.right);
        var X = Vec3.sum(A, Vec3.multiply(B, x));
        var d = Vec3.length(Vec3.subtract(P, X));
        var halfDiagonal = Vec3.length(properties.dimensions) / 2.0;

        var angularSize = 2 * Math.atan(halfDiagonal / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;
        if (0 < x && angularSize > MIN_ANGULAR_SIZE) {
            if (angularSize > MAX_ANGULAR_SIZE) {
                print("Angular size too big: " + angularSize);
                return { valid: false };
            }

            return { valid: true, x: x, y: y, z: z };
        }
        return { valid: false };
    }

    this.glowedIntersectingModel = { isKnownID: false };
    this.moveLaser = function () {
        // the overlays here are anchored to the avatar, which means they are specified in the avatar's local frame

        var inverseRotation = Quat.inverse(MyAvatar.orientation);
        var startPosition = Vec3.multiplyQbyV(inverseRotation, Vec3.subtract(this.palmPosition, MyAvatar.position));
        var direction = Vec3.multiplyQbyV(inverseRotation, Vec3.subtract(this.tipPosition, this.palmPosition));
        var distance = Vec3.length(direction);
        direction = Vec3.multiply(direction, LASER_LENGTH_FACTOR / distance);
        var endPosition = Vec3.sum(startPosition, direction);

        Overlays.editOverlay(this.laser, {
            position: startPosition,
            end: endPosition
        });


        Overlays.editOverlay(this.ball, {
            position: endPosition
        });
        Overlays.editOverlay(this.leftRight, {
            position: Vec3.sum(endPosition, Vec3.multiply(this.right, 2 * this.guideScale)),
            end: Vec3.sum(endPosition, Vec3.multiply(this.right, -2 * this.guideScale))
        });
        Overlays.editOverlay(this.topDown, { position: Vec3.sum(endPosition, Vec3.multiply(this.up, 2 * this.guideScale)),
            end: Vec3.sum(endPosition, Vec3.multiply(this.up, -2 * this.guideScale))
        });
        this.showLaser(!this.grabbing || mode == 0);

        if (this.glowedIntersectingModel.isKnownID) {
            Entities.editEntity(this.glowedIntersectingModel, { glowLevel: 0.0 });
            this.glowedIntersectingModel.isKnownID = false;
        }
        if (!this.grabbing) {
            var intersection = Entities.findRayIntersection({
                                                          origin: this.palmPosition,
                                                          direction: this.front
                                                          });
                                                          
            var halfDiagonal = Vec3.length(intersection.properties.dimensions) / 2.0;
                                                          
            var angularSize = 2 * Math.atan(halfDiagonal / Vec3.distance(Camera.getPosition(), intersection.properties.position)) * 180 / 3.14;
            if (intersection.accurate && intersection.entityID.isKnownID && angularSize > MIN_ANGULAR_SIZE && angularSize < MAX_ANGULAR_SIZE) {
                this.glowedIntersectingModel = intersection.entityID;
                Entities.editEntity(this.glowedIntersectingModel, { glowLevel: 0.25 });
            }
        }
    }

    this.showLaser = function (show) {
        Overlays.editOverlay(this.laser, { visible: show });
        Overlays.editOverlay(this.ball, { visible: show });
        Overlays.editOverlay(this.leftRight, { visible: show });
        Overlays.editOverlay(this.topDown, { visible: show });
    }
    this.moveEntity = function () {
        if (this.grabbing) {
            if (!this.entityID.isKnownID) {
                print("Unknown grabbed ID " + this.entityID.id + ", isKnown: " + this.entityID.isKnownID);
                this.entityID =  Entities.findRayIntersection({
                                                        origin: this.palmPosition,
                                                        direction: this.front
                                                           }).entityID;
                print("Identified ID " + this.entityID.id + ", isKnown: " + this.entityID.isKnownID);
            }
            var newPosition;
            var newRotation;

            switch (mode) {
                case 0:
                    newPosition = Vec3.sum(this.palmPosition,
                                           Vec3.multiply(this.front, this.x));
                    newPosition = Vec3.sum(newPosition,
                                           Vec3.multiply(this.up, this.y));
                    newPosition = Vec3.sum(newPosition,
                                           Vec3.multiply(this.right, this.z));


                    newRotation = Quat.multiply(this.rotation,
                                                Quat.inverse(this.oldRotation));
                    newRotation = Quat.multiply(newRotation,
                                                this.oldModelRotation);
                    break;
                case 1:
                    var forward = Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -1 });
                    var d = Vec3.dot(forward, MyAvatar.position);

                    var factor1 = Vec3.dot(forward, this.positionAtGrab) - d;
                    var factor2 = Vec3.dot(forward, this.modelPositionAtGrab) - d;
                    var vector = Vec3.subtract(this.palmPosition, this.positionAtGrab);

                    if (factor2 < 0) {
                        factor2 = 0;
                    }
                    if (factor1 <= 0) {
                        factor1 = 1;
                        factor2 = 1;
                    }

                    newPosition = Vec3.sum(this.modelPositionAtGrab,
                                           Vec3.multiply(vector,
                                                         factor2 / factor1));

                    newRotation = Quat.multiply(this.rotation,
                                                Quat.inverse(this.rotationAtGrab));
                    newRotation = Quat.multiply(newRotation,
                                                this.rotationAtGrab);
                    break;
            }
            Entities.editEntity(this.entityID, {
                             position: newPosition,
                             rotation: newRotation
                             });
            this.oldModelRotation = newRotation;
            this.oldModelPosition = newPosition;

            var indicesToRemove = [];
            for (var i = 0; i < this.jointsIntersectingFromStart.length; ++i) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(this.jointsIntersectingFromStart[i]), this.oldModelPosition);
                if (distance >= this.oldModelHalfDiagonal) {
                    indicesToRemove.push(this.jointsIntersectingFromStart[i]);
                }

            }
            for (var i = 0; i < indicesToRemove.length; ++i) {
                this.jointsIntersectingFromStart.splice(this.jointsIntersectingFromStart.indexOf(indicesToRemove[i], 1));
            }
        }
    }

    this.update = function () {
        this.oldPalmPosition = this.palmPosition;
        this.oldTipPosition = this.tipPosition;
        this.palmPosition = Controller.getSpatialControlPosition(this.palm);
        this.tipPosition = Controller.getSpatialControlPosition(this.tip);

        this.oldUp = this.up;
        this.up = Vec3.normalize(Controller.getSpatialControlNormal(this.palm));

        this.oldFront = this.front;
        this.front = Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition));

        this.oldRight = this.right;
        this.right = Vec3.normalize(Vec3.cross(this.front, this.up));

        this.oldRotation = this.rotation;
        this.rotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(this.palm));

        this.triggerValue = Controller.getTriggerValue(this.trigger);

        var bumperValue = Controller.isButtonPressed(this.bumper);
        if (bumperValue && !this.bumperValue) {
            if (mode == 0) {
                mode = 1;
                Overlays.editOverlay(leftController.laser, { color: { red: 0, green: 0, blue: 255 } });
                Overlays.editOverlay(rightController.laser, { color: { red: 0, green: 0, blue: 255 } });
            } else {
                mode = 0;
                Overlays.editOverlay(leftController.laser, { color: { red: 255, green: 0, blue: 0 } });
                Overlays.editOverlay(rightController.laser, { color: { red: 255, green: 0, blue: 0 } });
            }
        }
        this.bumperValue = bumperValue;


        this.checkTrigger();

        this.moveLaser();

        if (!this.pressed && this.grabbing) {
            // release if trigger not pressed anymore.
            this.release();
        }

        if (this.pressing) {
            // Checking for attachments intersecting
            var attachments = MyAvatar.getAttachmentData();
            var attachmentIndex = -1;
            var attachmentX = LASER_LENGTH_FACTOR;

            var newModel;
            var newProperties;

            for (var i = 0; i < attachments.length; ++i) {
                var position = Vec3.sum(MyAvatar.getJointPosition(attachments[i].jointName),
                                        Vec3.multiplyQbyV(MyAvatar.getJointCombinedRotation(attachments[i].jointName), attachments[i].translation));
                var scale = attachments[i].scale;

                var A = this.palmPosition;
                var B = this.front;
                var P = position;

                var x = Vec3.dot(Vec3.subtract(P, A), B);
                var X = Vec3.sum(A, Vec3.multiply(B, x));
                var d = Vec3.length(Vec3.subtract(P, X));

                if (d < scale / 2.0 && 0 < x && x < attachmentX) {
                    attachmentIndex = i;
                    attachmentX = d;
                }
            }

            if (attachmentIndex != -1) {
                print("Detaching: " + attachments[attachmentIndex].modelURL);
                MyAvatar.detachOne(attachments[attachmentIndex].modelURL, attachments[attachmentIndex].jointName);

                newProperties = {
                    type: "Model",
                    position: Vec3.sum(MyAvatar.getJointPosition(attachments[attachmentIndex].jointName),
                                       Vec3.multiplyQbyV(MyAvatar.getJointCombinedRotation(attachments[attachmentIndex].jointName), attachments[attachmentIndex].translation)),
                    rotation: Quat.multiply(MyAvatar.getJointCombinedRotation(attachments[attachmentIndex].jointName),
                                                 attachments[attachmentIndex].rotation),
                                                 
                    // TODO: how do we know the correct dimensions for detachment???
                    dimensions: { x: attachments[attachmentIndex].scale / 2.0,
                                  y: attachments[attachmentIndex].scale / 2.0,
                                  z: attachments[attachmentIndex].scale / 2.0 },
                                  
                    modelURL: attachments[attachmentIndex].modelURL
                };

                newModel = Entities.addEntity(newProperties);


            } else {
                // There is none so ...
                // Checking model tree
                Vec3.print("Looking at: ", this.palmPosition);
                var pickRay = { origin: this.palmPosition,
                    direction: Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition)) };
                var foundIntersection = Entities.findRayIntersection(pickRay);
                
                if(!foundIntersection.accurate) {
                    print("No accurate intersection");
                    return;
                }
                newModel = foundIntersection.entityID;
                if (!newModel.isKnownID) {
                    var identify = Entities.identifyEntity(newModel);
                    if (!identify.isKnownID) {
                        print("Unknown ID " + identify.id + " (update loop " + newModel.id + ")");
                        return;
                    }
                    newModel = identify;
                }
                newProperties = Entities.getEntityProperties(newModel);
            }
            print("foundEntity.modelURL=" + newProperties.modelURL);
            if (isLocked(newProperties)) {
                print("Model locked " + newProperties.id);
            } else {
                var check = this.checkEntity(newProperties);
                if (!check.valid) {
                    return;
                }

                this.grab(newModel, newProperties);

                this.x = check.x;
                this.y = check.y;
                this.z = check.z;
                return;
            }
        }
    }

    this.cleanup = function () {
        Overlays.deleteOverlay(this.laser);
        Overlays.deleteOverlay(this.ball);
        Overlays.deleteOverlay(this.leftRight);
        Overlays.deleteOverlay(this.topDown);
    }
}

var leftController = new controller(LEFT);
var rightController = new controller(RIGHT);

function moveEntities() {
    if (leftController.grabbing && rightController.grabbing && rightController.entityID.id == leftController.entityID.id) {
        var newPosition = leftController.oldModelPosition;
        var rotation = leftController.oldModelRotation;
        var ratio = 1;


        switch (mode) {
            case 0:
                var oldLeftPoint = Vec3.sum(leftController.oldPalmPosition, Vec3.multiply(leftController.oldFront, leftController.x));
                var oldRightPoint = Vec3.sum(rightController.oldPalmPosition, Vec3.multiply(rightController.oldFront, rightController.x));

                var oldMiddle = Vec3.multiply(Vec3.sum(oldLeftPoint, oldRightPoint), 0.5);
                var oldLength = Vec3.length(Vec3.subtract(oldLeftPoint, oldRightPoint));


                var leftPoint = Vec3.sum(leftController.palmPosition, Vec3.multiply(leftController.front, leftController.x));
                var rightPoint = Vec3.sum(rightController.palmPosition, Vec3.multiply(rightController.front, rightController.x));

                var middle = Vec3.multiply(Vec3.sum(leftPoint, rightPoint), 0.5);
                var length = Vec3.length(Vec3.subtract(leftPoint, rightPoint));


                ratio = length / oldLength;
                newPosition = Vec3.sum(middle,
                                       Vec3.multiply(Vec3.subtract(leftController.oldModelPosition, oldMiddle), ratio));
                break;
            case 1:
                var u = Vec3.normalize(Vec3.subtract(rightController.oldPalmPosition, leftController.oldPalmPosition));
                var v = Vec3.normalize(Vec3.subtract(rightController.palmPosition, leftController.palmPosition));

                var cos_theta = Vec3.dot(u, v);
                if (cos_theta > 1) {
                    cos_theta = 1;
                }
                var angle = Math.acos(cos_theta) / Math.PI * 180;
                if (angle < 0.1) {
                    return;

                }
                var w = Vec3.normalize(Vec3.cross(u, v));

                rotation = Quat.multiply(Quat.angleAxis(angle, w), leftController.oldModelRotation);


                leftController.positionAtGrab = leftController.palmPosition;
                leftController.rotationAtGrab = leftController.rotation;
                leftController.modelPositionAtGrab = leftController.oldModelPosition;
                leftController.rotationAtGrab = rotation;
                rightController.positionAtGrab = rightController.palmPosition;
                rightController.rotationAtGrab = rightController.rotation;
                rightController.modelPositionAtGrab = rightController.oldModelPosition;
                rightController.rotationAtGrab = rotation;
                break;
        }
        Entities.editEntity(leftController.entityID, {
                         position: newPosition,
                         rotation: rotation,
                         // TODO: how do we know the correct dimensions for detachment???
                         //radius: leftController.oldModelHalfDiagonal * ratio
                         dimensions: { x: leftController.oldModelHalfDiagonal * ratio,
                                       y: leftController.oldModelHalfDiagonal * ratio,
                                       z: leftController.oldModelHalfDiagonal * ratio }


                         });
        leftController.oldModelPosition = newPosition;
        leftController.oldModelRotation = rotation;
        leftController.oldModelHalfDiagonal *= ratio;

        rightController.oldModelPosition = newPosition;
        rightController.oldModelRotation = rotation;
        rightController.oldModelHalfDiagonal *= ratio;
        return;
    }
    leftController.moveEntity();
    rightController.moveEntity();
}

var hydraConnected = false;
function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    if (!isActive) {
        // So that we hide the lasers bellow and keep updating the overlays position
        numberOfButtons = 0;
    }

    // this is expected for hydras
    if (numberOfButtons == 12 && numberOfTriggers == 2 && controllersPerTrigger == 2) {
        if (!hydraConnected) {
            hydraConnected = true;
        }

        leftController.update();
        rightController.update();
        moveEntities();
    } else {
        if (hydraConnected) {
            hydraConnected = false;

            leftController.showLaser(false);
            rightController.showLaser(false);
        }
    }
    toolBar.move();
    progressDialog.move();
}

var entitySelected = false;
var selectedEntityID;
var selectedEntityProperties;
var mouseLastPosition;
var orientation;
var intersection;


var SCALE_FACTOR = 200.0;

function rayPlaneIntersection(pickRay, point, normal) {
    var d = -Vec3.dot(point, normal);
    var t = -(Vec3.dot(pickRay.origin, normal) + d) / Vec3.dot(pickRay.direction, normal);

    return Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, t));
}

function Tooltip() {
    this.x = 285;
    this.y = 115;
    this.width = 500;
    this.height = 180; // 145;
    this.margin = 5;
    this.decimals = 3;

    this.textOverlay = Overlays.addOverlay("text", {
        x: this.x,
        y: this.y,
        width: this.width,
        height: this.height,
        margin: this.margin,
        text: "",
        color: { red: 128, green: 128, blue: 128 },
        alpha: 0.2,
        visible: false
    });
    this.show = function (doShow) {
        Overlays.editOverlay(this.textOverlay, { visible: doShow });
    }
    this.updateText = function(properties) {
        var angles = Quat.safeEulerAngles(properties.rotation);
        var text = "Entity Properties:\n"
        text += "type: " + properties.type + "\n"
        text += "X: " + properties.position.x.toFixed(this.decimals) + "\n"
        text += "Y: " + properties.position.y.toFixed(this.decimals) + "\n"
        text += "Z: " + properties.position.z.toFixed(this.decimals) + "\n"
        text += "Pitch: " + angles.x.toFixed(this.decimals) + "\n"
        text += "Yaw:  " + angles.y.toFixed(this.decimals) + "\n"
        text += "Roll:    " + angles.z.toFixed(this.decimals) + "\n"
        text += "Dimensions: " + properties.dimensions.x.toFixed(this.decimals) + ", "
                               + properties.dimensions.y.toFixed(this.decimals) + ", "
                               + properties.dimensions.z.toFixed(this.decimals) + "\n";

        text += "Natural Dimensions: " + properties.naturalDimensions.x.toFixed(this.decimals) + ", "
                                       + properties.naturalDimensions.y.toFixed(this.decimals) + ", "
                                       + properties.naturalDimensions.z.toFixed(this.decimals) + "\n";

        text += "ID: " + properties.id + "\n"
        if (properties.type == "Model") {
            text += "Model URL: " + properties.modelURL + "\n"
            text += "Animation URL: " + properties.animationURL + "\n"
            text += "Animation is playing: " + properties.animationIsPlaying + "\n"
            if (properties.sittingPoints && properties.sittingPoints.length > 0) {
                text += properties.sittingPoints.length + " Sitting points: "
                for (var i = 0; i < properties.sittingPoints.length; ++i) {
                    text += properties.sittingPoints[i].name + " "
                }
            } else {
                text += "No sitting points" + "\n"
            }
        }
        if (properties.lifetime > -1) {
            text += "Lifetime: " + properties.lifetime + "\n"
        }
        text += "Age: " + properties.ageAsText + "\n"
        text += "Script: " + properties.script + "\n"


        Overlays.editOverlay(this.textOverlay, { text: text });
    }

    this.cleanup = function () {
        Overlays.deleteOverlay(this.textOverlay);
    }
}
var tooltip = new Tooltip();

function mousePressEvent(event) {
    if (event.isAlt) {
        return;
    }

    mouseLastPosition = { x: event.x, y: event.y };
    entitySelected = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

    if (toolBar.mousePressEvent(event) || progressDialog.mousePressEvent(event)) {
        // Event handled; do nothing.
        return;
    } else {
        // If we aren't active and didn't click on an overlay: quit
        if (!isActive) {
            return;
        }

        var pickRay = Camera.computePickRay(event.x, event.y);
        Vec3.print("[Mouse] Looking at: ", pickRay.origin);
        var foundIntersection = Entities.findRayIntersection(pickRay);

        if(!foundIntersection.accurate) {
            return;
        }
        var foundEntity = foundIntersection.entityID;

        if (!foundEntity.isKnownID) {
            var identify = Entities.identifyEntity(foundEntity);
            if (!identify.isKnownID) {
                print("Unknown ID " + identify.id + " (update loop " + foundEntity.id + ")");
                return;
            }
            foundEntity = identify;
        }

        var properties = Entities.getEntityProperties(foundEntity);
        if (isLocked(properties)) {
            print("Model locked " + properties.id);
        } else {
            var halfDiagonal = Vec3.length(properties.dimensions) / 2.0;

            print("Checking properties: " + properties.id + " " + properties.isKnownID + " - Half Diagonal:" + halfDiagonal);
            //                P         P - Model
            //               /|         A - Palm
            //              / | d       B - unit vector toward tip
            //             /  |         X - base of the perpendicular line
            //            A---X----->B  d - distance fom axis
            //              x           x - distance from A
            //
            //            |X-A| = (P-A).B
            //            X == A + ((P-A).B)B
            //            d = |P-X|

            var A = pickRay.origin;
            var B = Vec3.normalize(pickRay.direction);
            var P = properties.position;

            var x = Vec3.dot(Vec3.subtract(P, A), B);
            var X = Vec3.sum(A, Vec3.multiply(B, x));
            var d = Vec3.length(Vec3.subtract(P, X));
            var halfDiagonal = Vec3.length(properties.dimensions) / 2.0;

            var angularSize = 2 * Math.atan(halfDiagonal / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;
            if (0 < x && angularSize > MIN_ANGULAR_SIZE) {
                if (angularSize < MAX_ANGULAR_SIZE) {
                    entitySelected = true;
                    selectedEntityID = foundEntity;
                    selectedEntityProperties = properties;
                    orientation = MyAvatar.orientation;
                    intersection = rayPlaneIntersection(pickRay, P, Quat.getFront(orientation));
                } else {
                    print("Angular size too big: " + angularSize);
                }
            }
        }
    }
    if (entitySelected) {
        selectedEntityProperties.oldDimensions = selectedEntityProperties.dimensions;
        selectedEntityProperties.oldPosition = {
            x: selectedEntityProperties.position.x,
            y: selectedEntityProperties.position.y,
            z: selectedEntityProperties.position.z,
        };
        selectedEntityProperties.oldRotation = {
            x: selectedEntityProperties.rotation.x,
            y: selectedEntityProperties.rotation.y,
            z: selectedEntityProperties.rotation.z,
            w: selectedEntityProperties.rotation.w,
        };
        selectedEntityProperties.glowLevel = 0.0;
        
        print("Clicked on " + selectedEntityID.id + " " +  entitySelected);
        tooltip.updateText(selectedEntityProperties);
        tooltip.show(true);
    }
}

var glowedEntityID = { id: -1, isKnownID: false };
var oldModifier = 0;
var modifier = 0;
var wasShifted = false;
function mouseMoveEvent(event) {
    if (event.isAlt || !isActive) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    if (!entitySelected) {
        var entityIntersection = Entities.findRayIntersection(pickRay);
        if (entityIntersection.accurate) {
            if(glowedEntityID.isKnownID && glowedEntityID.id != entityIntersection.entityID.id) {
                Entities.editEntity(glowedEntityID, { glowLevel: 0.0 });
                glowedEntityID.id = -1;
                glowedEntityID.isKnownID = false;
            }

            var halfDiagonal = Vec3.length(entityIntersection.properties.dimensions) / 2.0;
            
            var angularSize = 2 * Math.atan(halfDiagonal / Vec3.distance(Camera.getPosition(), 
                                            entityIntersection.properties.position)) * 180 / 3.14;
                                            
            if (entityIntersection.entityID.isKnownID && angularSize > MIN_ANGULAR_SIZE && angularSize < MAX_ANGULAR_SIZE) {
                Entities.editEntity(entityIntersection.entityID, { glowLevel: 0.25 });
                glowedEntityID = entityIntersection.entityID;
            }
        }
        return;
    }

    if (event.isLeftButton) {
        if (event.isRightButton) {
            modifier = 1; // Scale
        } else {
            modifier = 2; // Translate
        }
    } else if (event.isRightButton) {
        modifier = 3; // rotate
    } else {
        modifier = 0;
    }
    pickRay = Camera.computePickRay(event.x, event.y);
    if (wasShifted != event.isShifted || modifier != oldModifier) {
        selectedEntityProperties.oldDimensions = selectedEntityProperties.dimensions;
        
        selectedEntityProperties.oldPosition = {
            x: selectedEntityProperties.position.x,
            y: selectedEntityProperties.position.y,
            z: selectedEntityProperties.position.z,
        };
        selectedEntityProperties.oldRotation = {
            x: selectedEntityProperties.rotation.x,
            y: selectedEntityProperties.rotation.y,
            z: selectedEntityProperties.rotation.z,
            w: selectedEntityProperties.rotation.w,
        };
        orientation = MyAvatar.orientation;
        intersection = rayPlaneIntersection(pickRay,
                                            selectedEntityProperties.oldPosition,
                                            Quat.getFront(orientation));

        mouseLastPosition = { x: event.x, y: event.y };
        wasShifted = event.isShifted;
        oldModifier = modifier;
        return;
    }


    switch (modifier) {
        case 0:
            return;
        case 1:
            // Let's Scale
            selectedEntityProperties.dimensions = Vec3.multiply(selectedEntityProperties.dimensions,
                                              (1.0 + (mouseLastPosition.y - event.y) / SCALE_FACTOR));

            var halfDiagonal = Vec3.length(selectedEntityProperties.dimensions) / 2.0;

            if (halfDiagonal < 0.01) {
                print("Scale too small ... bailling.");
                return;
            }
            break;

        case 2:
            // Let's translate
            var newIntersection = rayPlaneIntersection(pickRay,
                                                       selectedEntityProperties.oldPosition,
                                                       Quat.getFront(orientation));
            var vector = Vec3.subtract(newIntersection, intersection)
            if (event.isShifted) {
                var i = Vec3.dot(vector, Quat.getRight(orientation));
                var j = Vec3.dot(vector, Quat.getUp(orientation));
                vector = Vec3.sum(Vec3.multiply(Quat.getRight(orientation), i),
                                  Vec3.multiply(Quat.getFront(orientation), j));
            }
            selectedEntityProperties.position = Vec3.sum(selectedEntityProperties.oldPosition, vector);
            break;
        case 3:
            // Let's rotate
            if (somethingChanged) {
                selectedEntityProperties.oldRotation.x = selectedEntityProperties.rotation.x;
                selectedEntityProperties.oldRotation.y = selectedEntityProperties.rotation.y;
                selectedEntityProperties.oldRotation.z = selectedEntityProperties.rotation.z;
                selectedEntityProperties.oldRotation.w = selectedEntityProperties.rotation.w;
                mouseLastPosition.x = event.x;
                mouseLastPosition.y = event.y;
                somethingChanged = false;
            }


            var pixelPerDegrees = windowDimensions.y / (1 * 360); // the entire height of the window allow you to make 2 full rotations

            //compute delta in pixel
            var cameraForward = Quat.getFront(Camera.getOrientation());
            var rotationAxis = (!zIsPressed && xIsPressed) ? { x: 1, y: 0, z: 0 } :
                               (!zIsPressed && !xIsPressed) ? { x: 0, y: 1, z: 0 } :
                                                              { x: 0, y: 0, z: 1 };
            rotationAxis = Vec3.multiplyQbyV(selectedEntityProperties.rotation, rotationAxis);
            var orthogonalAxis = Vec3.cross(cameraForward, rotationAxis);
            var mouseDelta = { x: event.x - mouseLastPosition
                .x, y: mouseLastPosition.y - event.y, z: 0 };
            var transformedMouseDelta = Vec3.multiplyQbyV(Camera.getOrientation(), mouseDelta);
            var delta = Math.floor(Vec3.dot(transformedMouseDelta, Vec3.normalize(orthogonalAxis)) / pixelPerDegrees);

            var STEP = 15;
            if (!event.isShifted) {
                delta = Math.round(delta / STEP) * STEP;
            }

            var rotation = Quat.fromVec3Degrees({
                                                x: (!zIsPressed && xIsPressed) ? delta : 0,   // x is pressed
                                                y: (!zIsPressed && !xIsPressed) ? delta : 0,   // neither is pressed
                                                z: (zIsPressed && !xIsPressed) ? delta : 0   // z is pressed
                                                });
            rotation = Quat.multiply(selectedEntityProperties.oldRotation, rotation);
            
            selectedEntityProperties.rotation.x = rotation.x;
            selectedEntityProperties.rotation.y = rotation.y;
            selectedEntityProperties.rotation.z = rotation.z;
            selectedEntityProperties.rotation.w = rotation.w;
            break;
    }
    
    Entities.editEntity(selectedEntityID, selectedEntityProperties);
    tooltip.updateText(selectedEntityProperties);
}


function mouseReleaseEvent(event) {
    if (event.isAlt || !isActive) {
        return;
    }
    if (entitySelected) {
        tooltip.show(false);
    }
    
    entitySelected = false;
    
    glowedEntityID.id = -1;
    glowedEntityID.isKnownID = false;
}

// In order for editVoxels and editModels to play nice together, they each check to see if a "delete" menu item already
// exists. If it doesn't they add it. If it does they don't. They also only delete the menu item if they were the one that
// added it.
var modelMenuAddedDelete = false;
function setupModelMenus() {
    print("setupModelMenus()");
    // adj our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Models", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Edit Properties...",
        shortcutKeyEvent: { text: "`" }, afterItem: "Models" });
    if (!Menu.menuItemExists("Edit", "Delete")) {
        print("no delete... adding ours");
        Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete",
            shortcutKeyEvent: { text: "backspace" }, afterItem: "Models" });
        modelMenuAddedDelete = true;
    } else {
        print("delete exists... don't add ours");
    }

    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Paste Models", shortcutKey: "CTRL+META+V", afterItem: "Edit Properties..." });

    Menu.addMenuItem({ menuName: "File", menuItemName: "Models", isSeparator: true, beforeItem: "Settings" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Export Models", shortcutKey: "CTRL+META+E", afterItem: "Models" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Import Models", shortcutKey: "CTRL+META+I", afterItem: "Export Models" });
}

function cleanupModelMenus() {
    Menu.removeSeparator("Edit", "Models");
    Menu.removeMenuItem("Edit", "Edit Properties...");
    if (modelMenuAddedDelete) {
        // delete our menuitems
        Menu.removeMenuItem("Edit", "Delete");
    }

    Menu.removeMenuItem("Edit", "Paste Models");

    Menu.removeSeparator("File", "Models");
    Menu.removeMenuItem("File", "Export Models");
    Menu.removeMenuItem("File", "Import Models");
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    progressDialog.cleanup();
    toolBar.cleanup();
    cleanupModelMenus();
    tooltip.cleanup();
    modelImporter.cleanup();
    if (exportMenu) {
        exportMenu.close();
    }
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

setupModelMenus();

var propertiesForEditedEntity;
var editEntityFormArray;
var editModelID = -1;
var dimensionX;
var dimensionY;
var dimensionZ;

function handeMenuEvent(menuItem) {
    print("menuItemEvent() in JS... menuItem=" + menuItem);
    if (menuItem == "Delete") {
        if (leftController.grabbing) {
            print("  Delete Entity.... leftController.entityID="+ leftController.entityID);
            Entities.deleteEntity(leftController.entityID);
            leftController.grabbing = false;
            if (glowedEntityID.id == leftController.entityID.id) {
                glowedEntityID = { id: -1, isKnownID: false };
            }
        } else if (rightController.grabbing) {
            print("  Delete Entity.... rightController.entityID="+ rightController.entityID);
            Entities.deleteEntity(rightController.entityID);
            rightController.grabbing = false;
            if (glowedEntityID.id == rightController.entityID.id) {
                glowedEntityID = { id: -1, isKnownID: false };
            }
        } else if (entitySelected) {
            print("  Delete Entity.... selectedEntityID="+ selectedEntityID);
            Entities.deleteEntity(selectedEntityID);
            entitySelected = false;
            if (glowedEntityID.id == selectedEntityID.id) {
                glowedEntityID = { id: -1, isKnownID: false };
            }
        } else {
            print("  Delete Entity.... not holding...");
        }
    } else if (menuItem == "Edit Properties...") {
        editModelID = -1;
        if (leftController.grabbing) {
            print("  Edit Properties.... leftController.entityID="+ leftController.entityID);
            editModelID = leftController.entityID;
        } else if (rightController.grabbing) {
            print("  Edit Properties.... rightController.entityID="+ rightController.entityID);
            editModelID = rightController.entityID;
        } else if (entitySelected) {
            print("  Edit Properties.... selectedEntityID="+ selectedEntityID);
            editModelID = selectedEntityID;
        } else {
            print("  Edit Properties.... not holding...");
        }
        if (editModelID != -1) {
            print("  Edit Properties.... about to edit properties...");

            propertiesForEditedEntity = Entities.getEntityProperties(editModelID);
            var properties = propertiesForEditedEntity;

            var array = new Array();
            var index = 0;
            var decimals = 3;
            if (properties.type == "Model") {
                array.push({ label: "Model URL:", value: properties.modelURL });
                index++;
                array.push({ label: "Animation URL:", value: properties.animationURL });
                index++;
                array.push({ label: "Animation is playing:", value: properties.animationIsPlaying });
                index++;
                array.push({ label: "Animation FPS:", value: properties.animationFPS });
                index++;
                array.push({ label: "Animation Frame:", value: properties.animationFrameIndex });
                index++;
            }
            array.push({ label: "Position:", type: "header" });
            index++;
            array.push({ label: "X:", value: properties.position.x.toFixed(decimals) });
            index++;
            array.push({ label: "Y:", value: properties.position.y.toFixed(decimals) });
            index++;
            array.push({ label: "Z:", value: properties.position.z.toFixed(decimals) });
            index++;

            array.push({ label: "Rotation:", type: "header" });
            index++;
            var angles = Quat.safeEulerAngles(properties.rotation);
            array.push({ label: "Pitch:", value: angles.x.toFixed(decimals) });
            index++;
            array.push({ label: "Yaw:", value: angles.y.toFixed(decimals) });
            index++;
            array.push({ label: "Roll:", value: angles.z.toFixed(decimals) });
            index++;

            array.push({ label: "Dimensions:", type: "header" });
            index++;
            array.push({ label: "Width:", value: properties.dimensions.x.toFixed(decimals) });
            dimensionX = index;
            index++;
            array.push({ label: "Height:", value: properties.dimensions.y.toFixed(decimals) });
            dimensionY = index;
            index++;
            array.push({ label: "Depth:", value: properties.dimensions.z.toFixed(decimals) });
            dimensionZ = index;
            index++;
            array.push({ label: "", type: "inlineButton", buttonLabel: "Reset to Natural Dimensions", name: "resetDimensions" });
            index++;

            array.push({ label: "Velocity:", type: "header" });
            index++;
            array.push({ label: "Linear X:", value: properties.velocity.x.toFixed(decimals) });
            index++;
            array.push({ label: "Linear Y:", value: properties.velocity.y.toFixed(decimals) });
            index++;
            array.push({ label: "Linear Z:", value: properties.velocity.z.toFixed(decimals) });
            index++;
            array.push({ label: "Angular Pitch:", value: properties.rotationalVelocity.x.toFixed(decimals) });
            index++;
            array.push({ label: "Angular Yaw:", value: properties.rotationalVelocity.y.toFixed(decimals) });
            index++;
            array.push({ label: "Angular Roll:", value: properties.rotationalVelocity.z.toFixed(decimals) });
            index++;
            array.push({ label: "Damping:", value: properties.damping.toFixed(decimals) });
            index++;

            array.push({ label: "Gravity X:", value: properties.gravity.x.toFixed(decimals) });
            index++;
            array.push({ label: "Gravity Y:", value: properties.gravity.y.toFixed(decimals) });
            index++;
            array.push({ label: "Gravity Z:", value: properties.gravity.z.toFixed(decimals) });
            index++;

            array.push({ label: "Lifetime:", value: properties.lifetime.toFixed(decimals) });
            index++;

            array.push({ label: "Visible:", value: properties.visible });
            index++;
            
            if (properties.type == "Box" || properties.type == "Sphere") {
                array.push({ label: "Color:", type: "header" });
                index++;
                array.push({ label: "Red:", value: properties.color.red });
                index++;
                array.push({ label: "Green:", value: properties.color.green });
                index++;
                array.push({ label: "Blue:", value: properties.color.blue });
                index++;
            }
            array.push({ button: "Cancel" });
            index++;

            editEntityFormArray = array;
            Window.nonBlockingForm("Edit Properties", array);
        }
    } else if (menuItem == "Paste Models") {
        modelImporter.paste();
    } else if (menuItem == "Export Models") {
        if (!exportMenu) {
            exportMenu = new ExportMenu({
                onClose: function () {
                    exportMenu = null;
                }
            });
        }
    } else if (menuItem == "Import Models") {
        modelImporter.doImport();
    }
    tooltip.show(false);
}
Menu.menuItemEvent.connect(handeMenuEvent);



// handling of inspect.js concurrence
var zIsPressed = false;
var xIsPressed = false;
var somethingChanged = false;
Controller.keyPressEvent.connect(function (event) {
    if ((event.text == "z" || event.text == "Z") && !zIsPressed) {
        zIsPressed = true;
        somethingChanged = true;
    }
    if ((event.text == "x" || event.text == "X") && !xIsPressed) {
        xIsPressed = true;
        somethingChanged = true;
    }

    // resets model orientation when holding with mouse
    if (event.text == "r" && entitySelected) {
        selectedEntityProperties.rotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 });
        Entities.editEntity(selectedEntityID, selectedEntityProperties);
        tooltip.updateText(selectedEntityProperties);
        somethingChanged = true;
    }
});

Controller.keyReleaseEvent.connect(function (event) {
    if (event.text == "z" || event.text == "Z") {
        zIsPressed = false;
        somethingChanged = true;
    }
    if (event.text == "x" || event.text == "X") {
        xIsPressed = false;
        somethingChanged = true;
    }
    // since sometimes our menu shortcut keys don't work, trap our menu items here also and fire the appropriate menu items
    if (event.text == "`") {
        handeMenuEvent("Edit Properties...");
    }
    if (event.text == "BACKSPACE") {
        handeMenuEvent("Delete");
    }
});

Window.inlineButtonClicked.connect(function (name) {
    if (name == "resetDimensions") {
        var decimals = 3;
        
        Window.reloadNonBlockingForm([
            { value: propertiesForEditedEntity.naturalDimensions.x.toFixed(decimals), oldIndex: dimensionX },
            { value: propertiesForEditedEntity.naturalDimensions.y.toFixed(decimals), oldIndex: dimensionY },
            { value: propertiesForEditedEntity.naturalDimensions.z.toFixed(decimals), oldIndex: dimensionZ }
        ]);
    }
});
Window.nonBlockingFormClosed.connect(function() {
    print("JAVASCRIPT.... nonBlockingFormClosed....");
    array = editEntityFormArray;
    if (Window.getNonBlockingFormResult(array)) {
        print("getNonBlockingFormResult()....");

        var properties = propertiesForEditedEntity;
        var index = 0;
        if (properties.type == "Model") {
            properties.modelURL = array[index++].value;
            properties.animationURL = array[index++].value;
            properties.animationIsPlaying = array[index++].value;
            properties.animationFPS = array[index++].value;
            properties.animationFrameIndex = array[index++].value;
        }
        index++; // skip header
        properties.position.x = array[index++].value;
        properties.position.y = array[index++].value;
        properties.position.z = array[index++].value;

        index++; // skip header
        var angles = Quat.safeEulerAngles(properties.rotation);
        angles.x = array[index++].value;
        angles.y = array[index++].value;
        angles.z = array[index++].value;
        properties.rotation = Quat.fromVec3Degrees(angles);

        index++; // skip header
        properties.dimensions.x = array[index++].value;
        properties.dimensions.y = array[index++].value;
        properties.dimensions.z = array[index++].value;
        index++; // skip reset button

        index++; // skip header
        properties.velocity.x = array[index++].value;
        properties.velocity.y = array[index++].value;
        properties.velocity.z = array[index++].value;

        properties.rotationalVelocity.x = array[index++].value;
        properties.rotationalVelocity.y = array[index++].value;
        properties.rotationalVelocity.z = array[index++].value;

        properties.damping = array[index++].value;
        properties.gravity.x = array[index++].value;
        properties.gravity.y = array[index++].value;
        properties.gravity.z = array[index++].value;
        properties.lifetime = array[index++].value;
        properties.visible = array[index++].value;

        if (properties.type == "Box" || properties.type == "Sphere") {
            index++; // skip header
            properties.color.red = array[index++].value;
            properties.color.green = array[index++].value;
            properties.color.blue = array[index++].value;
        }
        Entities.editEntity(editModelID, properties);
    }
    modelSelected = false;
});


