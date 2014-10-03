//
//  httpMultiPart.js
//  examples/libraries
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


httpMultiPart = (function () {
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