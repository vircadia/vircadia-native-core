//
//  JSBaker.cpp
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/18/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "JSBaker.h"

#include <QtNetwork/QNetworkReply>

#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <SharedUtil.h>
#include <PathUtils.h>

const int ASCII_CHARACTERS_UPPER_LIMIT = 126;

JSBaker::JSBaker(const QUrl& jsURL, const QString& bakedOutputDir) :
    _jsURL(jsURL),
    _bakedOutputDir(bakedOutputDir)
{
}

void JSBaker::bake() {
    qCDebug(js_baking) << "JS Baker " << _jsURL << "bake starting";

    // once our script is loaded, kick off a the processing
    connect(this, &JSBaker::originalScriptLoaded, this, &JSBaker::processScript);

    if (_originalScript.isEmpty()) {
        // first load the script (either locally or remotely)
        loadScript();
    } else {
        // we already have a script passed to us, use that
        processScript();
    }
}

void JSBaker::loadScript() {
    // check if the script is local or first needs to be downloaded
    if (_jsURL.isLocalFile()) {
        // load up the local file
        QFile localScript(_jsURL.toLocalFile());
        if (!localScript.open(QIODevice::ReadOnly | QIODevice::Text)) {
            handleError("Error opening " + _jsURL.fileName() + " for reading");
            return;
        }

        _originalScript = localScript.readAll();

        emit originalScriptLoaded();
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, NetworkingConstants::VIRCADIA_USER_AGENT);

        networkRequest.setUrl(_jsURL);

        qCDebug(js_baking) << "Downloading" << _jsURL;

        // kickoff the download, wait for slot to tell us it is done
        auto networkReply = networkAccessManager.get(networkRequest);
        connect(networkReply, &QNetworkReply::finished, this, &JSBaker::handleScriptNetworkReply);
    }
}

void JSBaker::handleScriptNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(js_baking) << "Downloaded script" << _jsURL;

        // store the original script so it can be passed along for the bake
        _originalScript = requestReply->readAll();

        emit originalScriptLoaded();
    } else {
        // add an error to our list stating that this script could not be downloaded
        handleError("Error downloading " + _jsURL.toString() + " - " + requestReply->errorString());
    }
}

void JSBaker::processScript() {
    // Read file into an array
    QByteArray outputJS;

    // Call baking on inputJS and store result in outputJS 
    bool success = bakeJS(_originalScript, outputJS);
    if (!success) {
        qCDebug(js_baking) << "Bake Failed";
        handleError("Unterminated multi-line comment");
        return;
    }

    // Bake Successful. Export the file
    auto fileName = _jsURL.fileName();
    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    auto bakedFilename = baseName + BAKED_JS_EXTENSION;

    _bakedJSFilePath = _bakedOutputDir + "/" + bakedFilename;

    QFile bakedFile;
    bakedFile.setFileName(_bakedJSFilePath);
    if (!bakedFile.open(QIODevice::WriteOnly)) {
        handleError("Error opening " + _bakedJSFilePath + " for writing");
        return;
    }

    bakedFile.write(outputJS);

    // Export successful
    _outputFiles.push_back(_bakedJSFilePath);
    qCDebug(js_baking) << "Exported" << _jsURL << "minified to" << _bakedJSFilePath;

    // emit signal to indicate the JS baking is finished
    emit finished();
}

bool JSBaker::bakeJS(const QByteArray& inputFile, QByteArray& outputFile) {
    // Read from inputFile and write to outputFile per character 
    QTextStream in(inputFile, QIODevice::ReadOnly);
    QTextStream out(&outputFile, QIODevice::WriteOnly);

    // Algorithm requires the knowledge of previous and next character for each character read
    QChar currentCharacter;
    QChar nextCharacter;
    // Initialize previousCharacter with new line
    QChar previousCharacter = '\n';

    in >> currentCharacter;

    while (!in.atEnd()) {
        in >> nextCharacter;

        if (currentCharacter == '\r') {
            out << '\n';
        } else if (currentCharacter == '/') {
            // Check if single line comment i.e. //
            if (nextCharacter == '/') {
                handleSingleLineComments(in);

                //Start fresh after handling comments
                previousCharacter = '\n';
                in >> currentCharacter;
                continue;
            } else if (nextCharacter == '*') {
                // Check if multi line comment i.e. /*
                bool success = handleMultiLineComments(in);
                if (!success) {
                    // Errors present return false
                    return false;
                }
                //Start fresh after handling comments
                previousCharacter = '\n';
                in >> currentCharacter;
                continue;
            } else {
                // If '/' is not followed by '/' or '*' print '/'
                out << currentCharacter;
            }
        } else if (isSpaceOrTab(currentCharacter)) {
            // Check if white space or tab

            // Skip multiple spaces or tabs
            while (isSpaceOrTab(nextCharacter)) {
                in >> nextCharacter;
                if (nextCharacter == '\n') {
                    break;
                }
            }

            // check if space can be omitted
            if (!canOmitSpace(previousCharacter, nextCharacter)) {
                out << ' ';
            }
        } else if (currentCharacter == '\n') {
            // Check if new line

            //Skip multiple new lines
            //Skip new line followed by space or tab
            while (nextCharacter == '\n' || isSpaceOrTab(nextCharacter)) {
                in >> nextCharacter;
            }

            // Check if new line can be omitted
            if (!canOmitNewLine(previousCharacter, nextCharacter)) {
                out << '\n';
            }
        } else if (isQuote(currentCharacter)) {
            // Print the current quote and nextCharacter as is
            out << currentCharacter;
            out << nextCharacter;

            // Store the type of quote we are processing
            QChar quote = currentCharacter;

            // Don't modify the quoted strings
            while (nextCharacter != quote) {
                in >> nextCharacter;
                out << nextCharacter;
            }

            //Start fresh after handling quoted strings
            previousCharacter = nextCharacter;
            in >> currentCharacter;
            continue;
        } else {
            // In all other cases write the currentCharacter to outputFile
            out << currentCharacter;
        }

        previousCharacter = currentCharacter;
        currentCharacter = nextCharacter;
    }

    //write currentCharacter to output file when nextCharacter reaches EOF
    if (currentCharacter != '\n') {
        out << currentCharacter;
    }

    // Successful bake. Return true
    return true;
}

void JSBaker::handleSingleLineComments(QTextStream& in) {
    QChar character;
    while (!in.atEnd()) {
        in >> character;
        if (character == '\n') {
            break;
        }
    }
}

bool JSBaker::handleMultiLineComments(QTextStream& in) {
    QChar character;
    while (!in.atEnd()) {
        in >> character;
        if (character == '*') {
            if (in.read(1) == "/") {
                return true;
            }
        }
    }
    return false;
}

bool JSBaker::canOmitSpace(QChar previousCharacter, QChar nextCharacter) {
    return(!((isAlphanum(previousCharacter) || isNonAscii(previousCharacter) || isSpecialCharacter(previousCharacter)) &&
        (isAlphanum(nextCharacter) || isNonAscii(nextCharacter) || isSpecialCharacter(nextCharacter)))
           );
}

bool JSBaker::canOmitNewLine(QChar previousCharacter, QChar nextCharacter) {
    return (!((isAlphanum(previousCharacter) || isNonAscii(previousCharacter) || isSpecialCharacterPrevious(previousCharacter)) &&
        (isAlphanum(nextCharacter) || isNonAscii(nextCharacter) || isSpecialCharacterNext(nextCharacter)))
            );
}

//Check if character is alphabet, number or one of the following: '_', '$', '\\' or a non-ASCII character
bool JSBaker::isAlphanum(QChar c) {
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
            || c == '_' || c == '$' || c == '\\' || c > ASCII_CHARACTERS_UPPER_LIMIT);
}

bool JSBaker::isNonAscii(QChar c) {
    return ((int)c.toLatin1() > ASCII_CHARACTERS_UPPER_LIMIT);
}

// If previous and next characters are special characters, don't omit space
bool JSBaker::isSpecialCharacter(QChar c) {
    return (c == '\'' || c == '$' || c == '_' || c == '/' || c== '+' || c == '-');
}

// If previous character is a special character, maybe don't omit new line (depends on next character as well)
bool JSBaker::isSpecialCharacterPrevious(QChar c) {
    return (c == '\'' || c == '$' || c == '_' || c == '}' || c == ']' || c == ')' || c == '+' || c == '-'
            || c == '"' || c == '\'');
}

// If next character is a special character, maybe don't omit new line (depends on previous character as well)
bool JSBaker::isSpecialCharacterNext(QChar c) {
    return (c == '\'' || c == '$' || c == '_' || c == '{' || c == '[' || c == '(' || c == '+' || c == '-');
}

// Check if white space or tab
bool JSBaker::isSpaceOrTab(QChar c) {
    return (c == ' ' || c == '\t');
}

// Check If the currentCharacter is " or ' or `
bool JSBaker::isQuote(QChar c) {
    return (c == '"' || c == '\'' || c == '`');
}
