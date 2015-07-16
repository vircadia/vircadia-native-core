


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
