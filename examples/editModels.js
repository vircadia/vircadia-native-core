//
//  editModels.js
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
var DEFAULT_RADIUS = 0.10;

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
            if (i <= 255) {
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
        fstBuffer,
        fbxBuffer,
        svoBuffer,
        mapping,
        geometry,
        API_URL = "https://data.highfidelity.io/api/v1/models",
        MODEL_URL = "http://public.highfidelity.io/models/content",
        NAME_FIELD = "name",
        SCALE_FIELD = "scale",
        FILENAME_FIELD = "filename",
        TEXDIR_FIELD = "texdir",
        MAX_TEXTURE_SIZE = 1024;

    function error(message) {
        print(message);
        Window.alert(message);
    }

    function resetDataObjects() {
        fstBuffer = null;
        fbxBuffer = null;
        svoBuffer = null;
        mapping = {};
        geometry = {};
        geometry.textures = [];
    }

    function readFile(filename) {
        var url = "file:///" + filename,
            req = new XMLHttpRequest();

        req.open("GET", url, false);
        req.responseType = "arraybuffer";
        req.send();
        if (req.status !== 200) {
            error("Could not read file: " + filename + " : " + req.status + " " + req.statusText);
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
            remainder;

        mapping = {};  // { name : value | name : { value : remainder } }
        lines = dv.string(0, dv.byteLength).split(/\r\n|\r|\n/);
        for (i = 0; i < lines.length; i += 1) {
            line = lines[i].trim();
            if (line.length > 0 && line[0] !== "#") {
                tokens = line.split(/\s*=\s*/);
                if (tokens.length > 1) {
                    name = tokens[0];
                    value = tokens[1];
                    if (tokens.length === 2) {
                        mapping[name] = value;
                    } else {
                        // We're only interested in the first two fields so put the rest in the remainder
                        remainder = tokens.slice(2, tokens.length).join(" = ");
                        if (mapping[name] === undefined) {
                            mapping[name] = {};
                        }
                        mapping[name][value] = remainder;
                    }
                }
            }
        }
    }

    function writeMapping(buffer) {
        var name,
            value,
            remainder,
            string = "";

        for (name in mapping) {
            if (mapping.hasOwnProperty(name)) {
                if (typeof mapping[name] === "string") {
                    string += (name + " = " + mapping[name] + "\n");
                } else {
                    for (value in mapping[name]) {
                        if (mapping[name].hasOwnProperty(value)) {
                            remainder = mapping[name][value];
                            if (remainder === null) {
                                remainder = "";
                            } else {
                                remainder = " = " + remainder;
                            }
                            string += (name + " = " + value + remainder + "\n");
                        }
                    }
                }
            }
        }

        buffer.buffer = string.toArrayBuffer();
    }

    function readGeometry(fbxBuffer) {
        var textures,
            view,
            index,
            EOF;

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
                filename,
                author;

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

            if (name === "relativefilename") {
                filename = view.string(index + 5, view.getUint32(index + 1, true)).fileName();
                if (!textures.hasOwnProperty(filename)) {
                    textures[filename] = "";
                    geometry.textures.push(filename);
                }

            } else if (name === "author") {
                author = view.string(index + 5, view.getUint32(index + 1, true));
                geometry.author = author;

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
                author,
                filename;

            view = new Uint8Array(fbxBuffer.buffer);
            viewLength = view.byteLength;
            charCodes = [];

            for (index = 0; index < viewLength; index += 1) {
                charCode = view[index];
                if (charCode === 10) {  // Can ignore EOF
                    line = String.fromCharCode.apply(String, charCodes).trim();

                    if (line.slice(0, 7).toLowerCase() === "author:") {
                        author = line.slice(line.indexOf("\""), line.lastIndexOf("\"") - line.length);
                        geometry.author = author;

                    }
                    if (line.slice(0, 17).toLowerCase() === "relativefilename:") {
                        filename = line.slice(line.indexOf("\""), line.lastIndexOf("\"") - line.length).fileName();
                        if (!textures.hasOwnProperty(filename)) {
                            textures[filename] = "";
                            geometry.textures.push(filename);
                        }
                    }

                    charCodes = [];
                } else {
                    charCodes.push(charCode);
                }
            }
        }

        if (view.string(0, 18) === "Kaydara FBX Binary") {

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
            svoFilename,
            fileType;

        print("Reading model file: " + modelFile);

        if (modelFile.toLowerCase().slice(-4) === ".fst") {
            fstBuffer = readFile(modelFile);
            if (fstBuffer === null) {
                return false;
            }
            readMapping(fstBuffer);
            fileType = mapping[FILENAME_FIELD].toLowerCase().fileType();
            if (mapping.hasOwnProperty(FILENAME_FIELD)) {
                if (fileType === "fbx") {
                    fbxFilename = modelFile.path() + "\\" + mapping[FILENAME_FIELD];
                } else if (fileType === "svo") {
                    svoFilename = modelFile.path() + "\\" + mapping[FILENAME_FIELD];
                } else {
                    error("Unrecognized model type in FST file!");
                    return false;
                }
            } else {
                error("Model file name not found in FST file!");
                return false;
            }

        } else if (modelFile.toLowerCase().slice(-4) === ".fbx") {
            fbxFilename = modelFile;
            mapping[FILENAME_FIELD] = modelFile.fileName();

        } else if (modelFile.toLowerCase().slice(-4) === ".svo") {
            svoFilename = modelFile;
            mapping[FILENAME_FIELD] = modelFile.fileName();

        } else {
            error("Unrecognized file type: " + modelFile);
            return false;
        }

        if (fbxFilename) {
            fbxBuffer = readFile(fbxFilename);
            if (fbxBuffer === null) {
                return false;
            }

            readGeometry(fbxBuffer);
        }

        if (svoFilename) {
            svoBuffer = readFile(svoFilename);
            if (svoBuffer === null) {
                return false;
            }
        }

        if (mapping.hasOwnProperty(SCALE_FIELD)) {
            mapping[SCALE_FIELD] = parseFloat(mapping[SCALE_FIELD]);
        } else {
            mapping[SCALE_FIELD] = (geometry.author === "www.makehuman.org" ? 150.0 : 15.0);
        }

        // Add any missing basic mappings
        if (!mapping.hasOwnProperty(NAME_FIELD)) {
            mapping[NAME_FIELD] = modelFile.fileName().fileBase();
        }
        if (!mapping.hasOwnProperty(TEXDIR_FIELD)) {
            mapping[TEXDIR_FIELD] = ".";
        }
        if (!mapping.hasOwnProperty(SCALE_FIELD)) {
            mapping[SCALE_FIELD] = 0.2;  // For SVO models.
        }

        return true;
    }

    function setProperties() {
        var form = [],
            decimals = 3,
            directory,
            displayAs,
            validateAs;

        print("Setting model properties");

        form.push({ label: "Name:", value: mapping[NAME_FIELD] });

        directory = modelFile.path() + "/" + mapping[TEXDIR_FIELD];
        displayAs = new RegExp("^" + modelFile.path().replace(/[\\\\\\\/]/, "[\\\\\\\/]") + "[\\\\\\\/](.*)");
        validateAs = new RegExp("^" + modelFile.path().replace(/[\\\\\\\/]/, "[\\\\\\\/]") + "([\\\\\\\/].*)?");

        form.push({
            label: "Texture directory:",
            directory: modelFile.path() + "/" + mapping[TEXDIR_FIELD],
            title: "Choose Texture Directory",
            displayAs: displayAs,
            validateAs: validateAs,
            errorMessage: "Texture directory must be subdirectory of model directory."
        });

        form.push({ label: "Scale:", value: mapping[SCALE_FIELD].toFixed(decimals) });
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
        mapping[SCALE_FIELD] = form[6].value;

        writeMapping(fstBuffer);

        return true;
    }

    function createHttpMessage() {
        var lodCount,
            lodFile,
            lodBuffer,
            textureBuffer,
            textureSourceFormat,
            textureTargetFormat,
            i;

        print("Preparing to send model");

        httpMultiPart.clear();

        // Model name
        if (mapping.hasOwnProperty(NAME_FIELD)) {
            httpMultiPart.add({
                name : "model_name",
                string : mapping[NAME_FIELD]
            });
        } else {
            error("Model name is missing");
            httpMultiPart.clear();
            return false;
        }

        // FST file
        if (fstBuffer) {
            httpMultiPart.add({
                name : "fst",
                buffer: fstBuffer
            });
        }

        // FBX file
        if (fbxBuffer) {
            httpMultiPart.add({
                name : "fbx",
                buffer: fbxBuffer
            });
        }

        // SVO file
        if (svoBuffer) {
            httpMultiPart.add({
                name : "svo",
                buffer: svoBuffer
            });
        }

        // LOD files
        lodCount = 0;
        for (lodFile in mapping.lod) {
            if (mapping.lod.hasOwnProperty(lodFile)) {
                lodBuffer = readFile(modelFile.path() + "\/" + lodFile);
                if (lodBuffer === null) {
                    return false;
                }
                httpMultiPart.add({
                    name: "lod" + lodCount,
                    buffer: lodBuffer
                });
                lodCount += 1;
            }
        }

        // Textures
        for (i = 0; i < geometry.textures.length; i += 1) {
            textureBuffer = readFile(modelFile.path() + "\/"
                + (mapping[TEXDIR_FIELD] !== "." ? mapping[TEXDIR_FIELD] + "\/" : "")
                + geometry.textures[i]);
            if (textureBuffer === null) {
                return false;
            }

            textureSourceFormat = geometry.textures[i].fileType().toLowerCase();
            textureTargetFormat = (textureSourceFormat === "jpg" ? "jpg" : "png");
            textureBuffer.buffer = textureBuffer.buffer.recodeImage(textureSourceFormat, textureTargetFormat, MAX_TEXTURE_SIZE);
            textureBuffer.filename = textureBuffer.filename.slice(0, -textureSourceFormat.length) + textureTargetFormat;

            httpMultiPart.add({
                name: "texture" + i,
                buffer: textureBuffer
            });
        }

        // Model category
        httpMultiPart.add({
            name : "model_category",
            string : "content"
        });

        return true;
    }

    function sendToHighFidelity(addModelCallback) {
        var req,
            modelName,
            modelURL,
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
            print("Checking uploaded model");

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
                    if (Window.confirm("Your model has been uploaded as: " + modelURL + "\nDo you want to rez it?")) {
                        addModelCallback(modelURL);
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
                    uploadedChecks = 30;
                    checkUploaded();
                } else {
                    print("Error: " + req.status + " " + req.statusText);
                    error("There was a problem with your upload, please try again later.");
                }
            }
        };

        function requestUpload() {
            var url;

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

        print("Sending model to High Fidelity");

        modelName = mapping[NAME_FIELD];
        modelURL = MODEL_URL + "\/" + mapping[NAME_FIELD] + ".fst";  // DJRTODO: Do all models get a FST?

        requestUpload();
    }

    that.upload = function (file, addModelCallback) {

        modelFile = file;

        resetDataObjects();

        // Read model content ...
        if (!readModel()) {
            resetDataObjects();
            return;
        }

        // Set model properties ...
        if (!setProperties()) {
            resetDataObjects();
            return;
        }

        // Put model in HTTP message ...
        if (!createHttpMessage()) {
            resetDataObjects();
            return;
        }

        // Send model to High Fidelity ...
        sendToHighFidelity(addModelCallback);

        resetDataObjects();
    };

    return that;
}());

var toolBar = (function () {
    var that = {},
        toolBar,
        newModelButton,
        browseModelsButton,
        loadURLMenuItem,
        loadFileMenuItem,
        menuItemWidth = 90,
        menuItemOffset = 2,
        menuItemHeight = Tool.IMAGE_HEIGHT / 2 - menuItemOffset,
        menuItemMargin = 5,
        menuTextColor = { red: 255, green: 255, blue: 255 },
        menuBackgoundColor = { red: 18, green: 66, blue: 66 };

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL);

        newModelButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-model-tool.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        }, true);

        browseModelsButton = toolBar.addTool({
            imageURL: toolIconUrl + "list-icon.png",
            width: toolWidth,
            height: toolHeight,
            alpha: 0.7,
            visible: true
        });

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
    }

    function toggleToolbar(active) {
        if (active === undefined) {
            active = toolBar.toolSelected(newModelButton);
        } else {
            toolBar.selectTool(newModelButton, active);
        }

        Overlays.editOverlay(loadURLMenuItem, { visible: active });
        Overlays.editOverlay(loadFileMenuItem, { visible: active });
    }

    function addModel(url) {
        var position;

        position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

        if (position.x > 0 && position.y > 0 && position.z > 0) {
            Models.addModel({
                position: position,
                radius: DEFAULT_RADIUS,
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

        if (newModelButton === toolBar.clicked(clickedOverlay)) {
            toggleToolbar();
            return true;
        }

        if (clickedOverlay === loadURLMenuItem) {
            toggleToolbar(false);
            url = Window.prompt("Model URL", modelURLs[Math.floor(Math.random() * modelURLs.length)]);
            if (url !== null && url !== "") {
                addModel(url);
            }
            return true;
        }

        if (clickedOverlay === loadFileMenuItem) {
            toggleToolbar(false);
            file = Window.browse("Select your model file ...",
                Settings.getValue("LastModelUploadLocation").path(),
                "Model files (*.fst *.fbx *.svo)");
            if (file !== null) {
                Settings.setValue("LastModelUploadLocation", file);
                modelUploader.upload(file, addModel);
            }
            return true;
        }

        if (browseModelsButton === toolBar.clicked(clickedOverlay)) {
            toggleToolbar(false);
            url = Window.s3Browse();
            if (url !== null && url !== "") {
                addModel(url);
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
    this.modelID = { isKnownID: false };
    this.modelURL = "";
    this.oldModelRotation;
    this.oldModelPosition;
    this.oldModelRadius;
    
    this.positionAtGrab;
    this.rotationAtGrab;
    this.modelPositionAtGrab;
    this.modelRotationAtGrab;
    
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
    

    
    this.grab = function (modelID, properties) {
        if (isLocked(properties)) {
            print("Model locked " + modelID.id);
        } else {
            print("Grabbing " + modelID.id);
        
            this.grabbing = true;
            this.modelID = modelID;
            this.modelURL = properties.modelURL;
        
            this.oldModelPosition = properties.position;
            this.oldModelRotation = properties.modelRotation;
            this.oldModelRadius = properties.radius;
            
            this.positionAtGrab = this.palmPosition;
            this.rotationAtGrab = this.rotation;
            this.modelPositionAtGrab = properties.position;
            this.modelRotationAtGrab = properties.modelRotation;
            
            this.jointsIntersectingFromStart = [];
            for (var i = 0; i < jointList.length; i++) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(jointList[i]), this.oldModelPosition);
                if (distance < this.oldModelRadius) {
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
                print("closestJointDistance (attach max distance): " + closestJointDistance + " (" + this.oldModelRadius + ")");
            }
            
            if (closestJointDistance < this.oldModelRadius) {
                
                if (this.jointsIntersectingFromStart.indexOf(closestJointIndex) != -1 ||
                    (leftController.grabbing && rightController.grabbing &&
                    leftController.modelID.id == rightController.modelID.id)) {
                    // Do nothing
                } else {
                    print("Attaching to " + jointList[closestJointIndex]);
                    var jointPosition = MyAvatar.getJointPosition(jointList[closestJointIndex]);
                    var jointRotation = MyAvatar.getJointCombinedRotation(jointList[closestJointIndex]);
                    
                    var attachmentOffset = Vec3.subtract(this.oldModelPosition, jointPosition);
                    attachmentOffset = Vec3.multiplyQbyV(Quat.inverse(jointRotation), attachmentOffset);
                    var attachmentRotation = Quat.multiply(Quat.inverse(jointRotation), this.oldModelRotation);
                    
                    MyAvatar.attach(this.modelURL, jointList[closestJointIndex],
                                    attachmentOffset, attachmentRotation, 2.0 * this.oldModelRadius,
                                    true, false);
                    
                    Models.deleteModel(this.modelID);
                }
            }
        }
        
        this.grabbing = false;
        this.modelID.isKnownID = false;
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

    this.checkModel = function (properties) {
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
        
        var angularSize = 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;
        if (0 < x && angularSize > MIN_ANGULAR_SIZE) {
            if (angularSize > MAX_ANGULAR_SIZE) {
                print("Angular size too big: " + 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14);
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
        Overlays.editOverlay(this.topDown, {position: Vec3.sum(endPosition, Vec3.multiply(this.up, 2 * this.guideScale)),
                             end: Vec3.sum(endPosition, Vec3.multiply(this.up, -2 * this.guideScale))
                             });
        this.showLaser(!this.grabbing || mode == 0);
        
        if (this.glowedIntersectingModel.isKnownID) {
            Models.editModel(this.glowedIntersectingModel, { glowLevel: 0.0 });
            this.glowedIntersectingModel.isKnownID = false;
        }
        if (!this.grabbing) {
            var intersection = Models.findRayIntersection({
                                                          origin: this.palmPosition,
                                                          direction: this.front
                                                          });
            var angularSize = 2 * Math.atan(intersection.modelProperties.radius / Vec3.distance(Camera.getPosition(), intersection.modelProperties.position)) * 180 / 3.14;
            if (intersection.accurate && intersection.modelID.isKnownID && angularSize > MIN_ANGULAR_SIZE && angularSize < MAX_ANGULAR_SIZE) {
                this.glowedIntersectingModel = intersection.modelID;
                Models.editModel(this.glowedIntersectingModel, { glowLevel: 0.25 });
            }
        }
    }
    
    this.showLaser = function(show) {
        Overlays.editOverlay(this.laser, { visible: show });
        Overlays.editOverlay(this.ball, { visible: show });
        Overlays.editOverlay(this.leftRight, { visible: show });
        Overlays.editOverlay(this.topDown, { visible: show });
    }
    
    this.moveModel = function () {
        if (this.grabbing) {
            if (!this.modelID.isKnownID) {
                print("Unknown grabbed ID " + this.modelID.id + ", isKnown: " + this.modelID.isKnownID);
                this.modelID =  Models.findRayIntersection({
                                                        origin: this.palmPosition,
                                                        direction: this.front
                                                           }).modelID;
                print("Identified ID " + this.modelID.id + ", isKnown: " + this.modelID.isKnownID);
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
                                                this.modelRotationAtGrab);
                    break;
            }
            
            Models.editModel(this.modelID, {
                             position: newPosition,
                             modelRotation: newRotation
                             });
            
            this.oldModelRotation = newRotation;
            this.oldModelPosition = newPosition;
            
            var indicesToRemove = [];
            for (var i = 0; i < this.jointsIntersectingFromStart.length; ++i) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(this.jointsIntersectingFromStart[i]), this.oldModelPosition);
                if (distance >= this.oldModelRadius) {
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
                position: Vec3.sum(MyAvatar.getJointPosition(attachments[attachmentIndex].jointName),
                                   Vec3.multiplyQbyV(MyAvatar.getJointCombinedRotation(attachments[attachmentIndex].jointName), attachments[attachmentIndex].translation)),
                modelRotation: Quat.multiply(MyAvatar.getJointCombinedRotation(attachments[attachmentIndex].jointName),
                                             attachments[attachmentIndex].rotation),
                radius: attachments[attachmentIndex].scale / 2.0,
                modelURL: attachments[attachmentIndex].modelURL
                };
                newModel = Models.addModel(newProperties);
            } else {
                // There is none so ...
                // Checking model tree
                Vec3.print("Looking at: ", this.palmPosition);
                var pickRay = { origin: this.palmPosition,
                    direction: Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition)) };
                var foundIntersection = Models.findRayIntersection(pickRay);
                
                if(!foundIntersection.accurate) {
                    print("No accurate intersection");
                    return;
                }
                newModel = foundIntersection.modelID;
                
                if (!newModel.isKnownID) {
                    var identify = Models.identifyModel(newModel);
                    if (!identify.isKnownID) {
                        print("Unknown ID " + identify.id + " (update loop " + newModel.id + ")");
                        return;
                    }
                    newModel = identify;
                }
                newProperties = Models.getModelProperties(newModel);
            }
            
            
            print("foundModel.modelURL=" + newProperties.modelURL);
            
            if (isLocked(newProperties)) {
                print("Model locked " + newProperties.id);
            } else {
                var check = this.checkModel(newProperties);
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

function moveModels() {
    if (leftController.grabbing && rightController.grabbing && rightController.modelID.id == leftController.modelID.id) {
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
                leftController.modelRotationAtGrab = rotation;
                
                rightController.positionAtGrab = rightController.palmPosition;
                rightController.rotationAtGrab = rightController.rotation;
                rightController.modelPositionAtGrab = rightController.oldModelPosition;
                rightController.modelRotationAtGrab = rotation;
                break;
        }
        
        Models.editModel(leftController.modelID, {
                         position: newPosition,
                         modelRotation: rotation,
                         radius: leftController.oldModelRadius * ratio
                         });
        
        leftController.oldModelPosition = newPosition;
        leftController.oldModelRotation = rotation;
        leftController.oldModelRadius *= ratio;
        
        rightController.oldModelPosition = newPosition;
        rightController.oldModelRotation = rotation;
        rightController.oldModelRadius *= ratio;
        return;
    }
    
    leftController.moveModel();
    rightController.moveModel();
}

var hydraConnected = false;
function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;
    
    // this is expected for hydras
    if (numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2) {
        if (!hydraConnected) {
            hydraConnected = true;
        }
        
        leftController.update();
        rightController.update();
        moveModels();
    } else {
        if (hydraConnected) {
            hydraConnected = false;
            
            leftController.showLaser(false);
            rightController.showLaser(false);
        }
    }
    
    toolBar.move();
}



var modelSelected = false;
var selectedModelID;
var selectedModelProperties;
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
    this.height = 145 ;
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
    this.show = function(doShow) {
        Overlays.editOverlay(this.textOverlay, { visible: doShow });
    }
    this.updateText = function(properties) {
        var angles = Quat.safeEulerAngles(properties.modelRotation);
        var text = "Model Properties:\n"
        text += "x: " + properties.position.x.toFixed(this.decimals) + "\n"
        text += "y: " + properties.position.y.toFixed(this.decimals) + "\n"
        text += "z: " + properties.position.z.toFixed(this.decimals) + "\n"
        text += "pitch: " + angles.x.toFixed(this.decimals) + "\n"
        text += "yaw:  " + angles.y.toFixed(this.decimals) + "\n"
        text += "roll:    " + angles.z.toFixed(this.decimals) + "\n"
        text += "Scale: " + 2 * properties.radius.toFixed(this.decimals) + "\n"
        text += "ID: " + properties.id + "\n"
        text += "model url: " + properties.modelURL + "\n"
        text += "animation url: " + properties.animationURL + "\n"
        if (properties.sittingPoints.length > 0) {
            text += properties.sittingPoints.length + " sitting points: "
            for (var i = 0; i < properties.sittingPoints.length; ++i) {
                text += properties.sittingPoints[i].name + " "
            }
        } else {
            text += "No sitting points"
        }


        Overlays.editOverlay(this.textOverlay, { text: text });
    }
    
    this.cleanup = function() {
        Overlays.deleteOverlay(this.textOverlay);
    }
}
var tooltip = new Tooltip();

function mousePressEvent(event) {
    if (event.isAlt) {
        return;
    }
    
    mouseLastPosition = { x: event.x, y: event.y };
    modelSelected = false;
    
    if (toolBar.mousePressEvent(event)) {
        // Event handled; do nothing.
    } else {
        var pickRay = Camera.computePickRay(event.x, event.y);
        Vec3.print("[Mouse] Looking at: ", pickRay.origin);
        var foundIntersection = Models.findRayIntersection(pickRay);
        
        if(!foundIntersection.accurate) {
            return;
        }
        var foundModel = foundIntersection.modelID;
        
        if (!foundModel.isKnownID) {
            var identify = Models.identifyModel(foundModel);
            if (!identify.isKnownID) {
                print("Unknown ID " + identify.id + " (update loop " + foundModel.id + ")");
                return;
            }
            foundModel = identify;
        }
        
        var properties = Models.getModelProperties(foundModel);
        if (isLocked(properties)) {
            print("Model locked " + properties.id);
        } else {
            print("Checking properties: " + properties.id + " " + properties.isKnownID);
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
            
            var angularSize = 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;
            if (0 < x && angularSize > MIN_ANGULAR_SIZE) {
                if (angularSize < MAX_ANGULAR_SIZE) {
                    modelSelected = true;
                    selectedModelID = foundModel;
                    selectedModelProperties = properties;
                    
                    orientation = MyAvatar.orientation;
                    intersection = rayPlaneIntersection(pickRay, P, Quat.getFront(orientation));
                } else {
                    print("Angular size too big: " + 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14);
                }
            }
        }
    }
    
    if (modelSelected) {
        selectedModelProperties.oldRadius = selectedModelProperties.radius;
        selectedModelProperties.oldPosition = {
        x: selectedModelProperties.position.x,
        y: selectedModelProperties.position.y,
        z: selectedModelProperties.position.z,
        };
        selectedModelProperties.oldRotation = {
        x: selectedModelProperties.modelRotation.x,
        y: selectedModelProperties.modelRotation.y,
        z: selectedModelProperties.modelRotation.z,
        w: selectedModelProperties.modelRotation.w,
        };
        selectedModelProperties.glowLevel = 0.0;
        
        print("Clicked on " + selectedModelID.id + " " +  modelSelected);
        tooltip.updateText(selectedModelProperties);
        tooltip.show(true);
    }
}

var glowedModelID = { id: -1, isKnownID: false };
var oldModifier = 0;
var modifier = 0;
var wasShifted = false;
function mouseMoveEvent(event)  {
    if (event.isAlt) {
        return;
    }
    
    var pickRay = Camera.computePickRay(event.x, event.y);
    
    if (!modelSelected) {
        var modelIntersection = Models.findRayIntersection(pickRay);
        if (modelIntersection.accurate) {
            if(glowedModelID.isKnownID && glowedModelID.id != modelIntersection.modelID.id) {
                Models.editModel(glowedModelID, { glowLevel: 0.0 });
                glowedModelID.id = -1;
                glowedModelID.isKnownID = false;
            }
            
            var angularSize = 2 * Math.atan(modelIntersection.modelProperties.radius / Vec3.distance(Camera.getPosition(), modelIntersection.modelProperties.position)) * 180 / 3.14;
            if (modelIntersection.modelID.isKnownID && angularSize > MIN_ANGULAR_SIZE && angularSize < MAX_ANGULAR_SIZE) {
                Models.editModel(modelIntersection.modelID, { glowLevel: 0.25 });
                glowedModelID = modelIntersection.modelID;
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
        selectedModelProperties.oldRadius = selectedModelProperties.radius;
        
        selectedModelProperties.oldPosition = {
        x: selectedModelProperties.position.x,
        y: selectedModelProperties.position.y,
        z: selectedModelProperties.position.z,
        };
        selectedModelProperties.oldRotation = {
        x: selectedModelProperties.modelRotation.x,
        y: selectedModelProperties.modelRotation.y,
        z: selectedModelProperties.modelRotation.z,
        w: selectedModelProperties.modelRotation.w,
        };
        orientation = MyAvatar.orientation;
        intersection = rayPlaneIntersection(pickRay,
                                            selectedModelProperties.oldPosition,
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
            selectedModelProperties.radius = (selectedModelProperties.oldRadius *
                                              (1.0 + (mouseLastPosition.y - event.y) / SCALE_FACTOR));
            
            if (selectedModelProperties.radius < 0.01) {
                print("Scale too small ... bailling.");
                return;
            }
            break;
            
        case 2:
            // Let's translate
            var newIntersection = rayPlaneIntersection(pickRay,
                                                       selectedModelProperties.oldPosition,
                                                       Quat.getFront(orientation));
            var vector = Vec3.subtract(newIntersection, intersection)
            if (event.isShifted) {
                var i = Vec3.dot(vector, Quat.getRight(orientation));
                var j = Vec3.dot(vector, Quat.getUp(orientation));
                vector = Vec3.sum(Vec3.multiply(Quat.getRight(orientation), i),
                                  Vec3.multiply(Quat.getFront(orientation), j));
            }
            
            selectedModelProperties.position = Vec3.sum(selectedModelProperties.oldPosition, vector);
            break;
        case 3:
            // Let's rotate
            if (somethingChanged) {
                selectedModelProperties.oldRotation.x = selectedModelProperties.modelRotation.x;
                selectedModelProperties.oldRotation.y = selectedModelProperties.modelRotation.y;
                selectedModelProperties.oldRotation.z = selectedModelProperties.modelRotation.z;
                selectedModelProperties.oldRotation.w = selectedModelProperties.modelRotation.w;
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
            rotationAxis = Vec3.multiplyQbyV(selectedModelProperties.modelRotation, rotationAxis);
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
            rotation = Quat.multiply(selectedModelProperties.oldRotation, rotation);
            
            selectedModelProperties.modelRotation.x = rotation.x;
            selectedModelProperties.modelRotation.y = rotation.y;
            selectedModelProperties.modelRotation.z = rotation.z;
            selectedModelProperties.modelRotation.w = rotation.w;
            break;
    }
    
    Models.editModel(selectedModelID, selectedModelProperties);
    tooltip.updateText(selectedModelProperties);
}


function mouseReleaseEvent(event) {
    if (event.isAlt) {
        return;
    }
    
    if (modelSelected) {
        tooltip.show(false);
    }
    
    modelSelected = false;
    
    glowedModelID.id = -1;
    glowedModelID.isKnownID = false;
}

// In order for editVoxels and editModels to play nice together, they each check to see if a "delete" menu item already
// exists. If it doesn't they add it. If it does they don't. They also only delete the menu item if they were the one that
// added it.
var modelMenuAddedDelete = false;
function setupModelMenus() {
    print("setupModelMenus()");
    // add our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Models", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Edit Properties...", 
        shortcutKeyEvent: { text: "`" }, afterItem: "Models" });
    if (!Menu.menuItemExists("Edit","Delete")) {
        print("no delete... adding ours");
        Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete", 
            shortcutKeyEvent: { text: "backspace" }, afterItem: "Models" });
        modelMenuAddedDelete = true;
    } else {
        print("delete exists... don't add ours");
    }
}

function cleanupModelMenus() {
    Menu.removeSeparator("Edit", "Models");
    Menu.removeMenuItem("Edit", "Edit Properties...");
    if (modelMenuAddedDelete) {
        // delete our menuitems
        Menu.removeMenuItem("Edit", "Delete");
    }
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    toolBar.cleanup();
    cleanupModelMenus();
    tooltip.cleanup();
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

setupModelMenus();

function handeMenuEvent(menuItem){
    print("menuItemEvent() in JS... menuItem=" + menuItem);
    if (menuItem == "Delete") {
        if (leftController.grabbing) {
            print("  Delete Model.... leftController.modelID="+ leftController.modelID);
            Models.deleteModel(leftController.modelID);
            leftController.grabbing = false;
        } else if (rightController.grabbing) {
            print("  Delete Model.... rightController.modelID="+ rightController.modelID);
            Models.deleteModel(rightController.modelID);
            rightController.grabbing = false;
        } else if (modelSelected) {
            print("  Delete Model.... selectedModelID="+ selectedModelID);
            Models.deleteModel(selectedModelID);
            modelSelected = false;
        } else {
            print("  Delete Model.... not holding...");
        }
    } else if (menuItem == "Edit Properties...") {
        var editModelID = -1;
        if (leftController.grabbing) {
            print("  Edit Properties.... leftController.modelID="+ leftController.modelID);
            editModelID = leftController.modelID;
        } else if (rightController.grabbing) {
            print("  Edit Properties.... rightController.modelID="+ rightController.modelID);
            editModelID = rightController.modelID;
        } else if (modelSelected) {
            print("  Edit Properties.... selectedModelID="+ selectedModelID);
            editModelID = selectedModelID;
        } else {
            print("  Edit Properties.... not holding...");
        }
        if (editModelID != -1) {
            print("  Edit Properties.... about to edit properties...");
            var array = new Array();
            var decimals = 3;
            array.push({ label: "Model URL:", value: selectedModelProperties.modelURL });
            array.push({ label: "Animation URL:", value: selectedModelProperties.animationURL });
            array.push({ label: "X:", value: selectedModelProperties.position.x.toFixed(decimals) });
            array.push({ label: "Y:", value: selectedModelProperties.position.y.toFixed(decimals) });
            array.push({ label: "Z:", value: selectedModelProperties.position.z.toFixed(decimals) });
            var angles = Quat.safeEulerAngles(selectedModelProperties.modelRotation);
            array.push({ label: "Pitch:", value: angles.x.toFixed(decimals) });
            array.push({ label: "Yaw:", value: angles.y.toFixed(decimals) });
            array.push({ label: "Roll:", value: angles.z.toFixed(decimals) });
            array.push({ label: "Scale:", value: 2 * selectedModelProperties.radius.toFixed(decimals) });
            array.push({ button: "Cancel" });

            if (Window.form("Edit Properties", array)) {
                selectedModelProperties.modelURL = array[0].value;
                selectedModelProperties.animationURL = array[1].value;
                selectedModelProperties.position.x = array[2].value;
                selectedModelProperties.position.y = array[3].value;
                selectedModelProperties.position.z = array[4].value;
                angles.x = array[5].value;
                angles.y = array[6].value;
                angles.z = array[7].value;
                selectedModelProperties.modelRotation = Quat.fromVec3Degrees(angles);
                selectedModelProperties.radius = array[8].value / 2;
                print(selectedModelProperties.radius);

                Models.editModel(selectedModelID, selectedModelProperties);
            }

            modelSelected = false;
        }
    }
    tooltip.show(false);
}
Menu.menuItemEvent.connect(handeMenuEvent);



// handling of inspect.js concurrence
var zIsPressed = false;
var xIsPressed = false;
var somethingChanged = false;
Controller.keyPressEvent.connect(function(event) {
    if ((event.text == "z" || event.text == "Z") && !zIsPressed) {
        zIsPressed = true;
        somethingChanged = true;
    }
    if ((event.text == "x" || event.text == "X") && !xIsPressed) {
        xIsPressed = true;
        somethingChanged = true;
    }
                                 
    // resets model orientation when holding with mouse
    if (event.text == "r" && modelSelected) {
        selectedModelProperties.modelRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 });
        Models.editModel(selectedModelID, selectedModelProperties);
        tooltip.updateText(selectedModelProperties);
        somethingChanged = true;
    }
});
Controller.keyReleaseEvent.connect(function(event) {
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