'use strict'

const request = require('request');
const extend = require('extend');
const util = require('util');
const events = require('events');
const childProcess = require('child_process');
const fs = require('fs-extra');
const os = require('os');
const path = require('path');

const hfApp = require('./hf-app.js');
const getInterfaceDataDirectory = hfApp.getInterfaceDataDirectory;


const VariantTypes = {
    USER_TYPE: 1024
}

function AccountInfo() {

    var accountInfoPath = path.join(getInterfaceDataDirectory(), '/AccountInfo.bin');
    this.rawData = null;
    this.parseOffset = 0;
    try {
        this.rawData = fs.readFileSync(accountInfoPath);

        this.data = this._parseMap();

    } catch(e) {
        console.log(e);
        log.debug("AccountInfo file not found: " + accountInfoPath);
    }
}

AccountInfo.prototype = {

    accessToken: function (metaverseUrl) {
        if (this.data && this.data[metaverseUrl] && this.data[metaverseUrl]["accessToken"]) {
            return this.data[metaverseUrl]["accessToken"]["token"];
        }
        return null;
    },
    _parseUInt32: function () {
        if (!this.rawData || (this.rawData.length - this.parseOffset < 4)) {
            throw "Expected uint32";
        }
        var result = this.rawData.readUInt32BE(this.parseOffset);
        this.parseOffset += 4;
        return result;
    },
    _parseMap: function () {
        var result = {};
        var n = this._parseUInt32();
        for (var i = 0; i < n; i++) {
            var key = this._parseQString();
            result[key] = this._parseVariant();
        }
        return result;
    },
    _parseVariant: function () {
        var varType = this._parseUInt32();
        var isNull = this.rawData[this.parseOffset++];

        switch (varType) {
            case VariantTypes.USER_TYPE:
              //user type
              var userTypeName = this._parseByteArray().toString('ascii').slice(0,-1);
              if (userTypeName === "DataServerAccountInfo") {
                  return this._parseDataServerAccountInfo();
              }
              else {
                  throw "Unknown custom type " + userTypeName;
              }
              break;
        }
    },
    _parseByteArray: function () {
        var length = this._parseUInt32();
        if (length === 0xffffffff) {
            return null;
        }
        var result = this.rawData.slice(this.parseOffset, this.parseOffset+length);
        this.parseOffset += length;
        return result;

    },
    _parseQString: function () {
        if (!this.rawData || (this.rawData.length <= this.parseOffset)) {
            throw "Expected QString";
        }
        // length in bytes;
        var length = this._parseUInt32();
        if (length === 0xFFFFFFFF) {
            return null;
        }

        if (this.rawData.length - this.parseOffset < length) {
            throw "Insufficient buffer length for QString parsing";
        }

        // Convert from BE UTF16 to LE
        var resultBuffer = this.rawData.slice(this.parseOffset, this.parseOffset+length);
        resultBuffer.swap16();
        var result = resultBuffer.toString('utf16le');
        this.parseOffset += length;
        return result;
    },
    _parseDataServerAccountInfo: function () {
        return {
            accessToken: this._parseOAuthAccessToken(),
            username: this._parseQString(),
            xmppPassword: this._parseQString(),
            discourseApiKey: this._parseQString(),
            walletId: this._parseUUID(),
            privateKey: this._parseByteArray(),
            domainId: this._parseUUID(),
            tempDomainId: this._parseUUID(),
            tempDomainApiKey: this._parseQString()

        }
    },
    _parseOAuthAccessToken: function () {
        return  {
            token: this._parseQString(),
            timestampHigh: this._parseUInt32(),
            timestampLow: this._parseUInt32(),
            tokenType: this._parseQString(),
            refreshToken: this._parseQString()
        }
    },
    _parseUUID: function () {
        this.parseOffset += 16;
        return null;
    }
}

exports.AccountInfo = AccountInfo;