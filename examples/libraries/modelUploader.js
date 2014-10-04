//
//  modelUploader.js
//  examples/libraries
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


modelUploader = (function () {
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