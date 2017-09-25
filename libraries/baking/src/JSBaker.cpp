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

#include <PathUtils.h> // To generate Temporary directory

#include "JSBaker.h"
#include "Baker.h"

JSBaker::JSBaker(const QUrl& jsURL, const QString& bakedOutputDir) :
    _jsURL(jsURL),
    _bakedOutputDir(bakedOutputDir)
{
    
};

void JSBaker::bake() {
    auto tempDir = PathUtils::generateTemporaryDir();
    
    if (tempDir.isEmpty()) {
        handleError("Failed to create a temporary directory.");
        return;
    }

    _tempDir = tempDir;
    
    _originalJSFilePath = _tempDir.filePath(_jsURL.fileName());
    qCDebug(js_baking) << "Made temporary dir " << _tempDir;
    qCDebug(js_baking) << "Origin file path: " << _originalJSFilePath;

    // When source JS is loaded, trigger the importJS method
    connect(this, &JSBaker::sourceJSLoaded, this, &JSBaker::importJS);

    // Make a local copy of the JS File
    loadSourceJS();
}

void JSBaker::loadSourceJS() {
    // load the local file
    QFile localJS{ _jsURL.toLocalFile() };
    
    if (!localJS.exists()) {
        
        handleError("Could not find " + _jsURL.toString());
        return;
    }

    // make a copy of local file at _originalJSFilePath
    qCDebug(js_baking) << "Local file url: " << _jsURL << _jsURL.toString() << _jsURL.toLocalFile() << ", copying to: " << _originalJSFilePath;
    localJS.copy(_originalJSFilePath);

    // emit signal to indicate script is ready to import
    emit sourceJSLoaded();
}

void JSBaker::importJS() {    
    // Import the file to be processed from _originalJSFilePath
    QFile jsFile(_originalJSFilePath);
    if (!jsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        handleError("Error opening " + _originalJSFilePath + " for reading");
    }

    if (hasErrors()) {
        // Return if cannot open file
        return;
    } else {
        // Import successful, Call the baking function with the imported file
        bakeJS(&jsFile);
    }
}

void JSBaker::bakeJS(QFile* inputFile) {
    // Create an output file which will be exported as the bakedFile
    QFile outputFile;
    outputFile.open(QIODevice::WriteOnly | QIODevice::Text);
    
    // Read from inputFile and write to outputFile per character 
    QTextStream readCharacter(inputFile);
    QTextStream writeCharacter(&outputFile);

    // Algorithm requires the knowledge of previous and next character for each character read
    QChar currentCharacter;
    QChar nextCharacter;
    // Initialize previousCharacter with new line
    QChar previousCharacter = '\n';
    
    // Input currentCharacter
    readCharacter >> currentCharacter;
    
    while (!readCharacter.atEnd()) {
        // input nextCharacter
        readCharacter >> nextCharacter;
        
        if (currentCharacter == '\r') {
            writeCharacter << '\n';
        } else if (currentCharacter == '/') {
            // Check if single line comment i.e. //
            if (nextCharacter == '/') {
                handleSingleLineComments(&readCharacter);
                
                //Start fresh after handling comments
                previousCharacter = '\n';
                readCharacter >> currentCharacter;
                continue;
            } else if (nextCharacter == '*') {
                // Check if multi line comment i.e. /*
                handleMultiLineComments(&readCharacter);
                
                //Start fresh after handling comments
                previousCharacter = '\n';
                readCharacter >> currentCharacter;
                continue;
            } else {
                // If '/' is not followed by '/' or '*' print '/'
                writeCharacter << currentCharacter;
            }
        } else if (isControlCharacter(currentCharacter)) { 
            // Check if white space or tab

            // Skip multiple spaces or tabs
            if (isControlCharacter(nextCharacter)) {
                while (isControlCharacter(nextCharacter)) {
                    readCharacter >> nextCharacter;
                    if (nextCharacter == '\n') {
                        break;
                    }
                }
            }
            
            // check if space can be omitted
            if (!canOmitSpace(previousCharacter, nextCharacter)) {
                writeCharacter << ' ';
            }
        } else if (currentCharacter == '\n') { 
            // Check if new line
            
            //Skip multiple new lines
            //Skip new line followed by space or tab
            if (nextCharacter == '\n' || isControlCharacter(nextCharacter)) { 
                while (nextCharacter == '\n' || isControlCharacter(nextCharacter)) {
                    readCharacter >> nextCharacter;
                }
            } 
            // Check if new line can be omitted
            if (!canOmitNewLine(previousCharacter, nextCharacter)) {
                writeCharacter << '\n';
            }
        } else if (isQuote(currentCharacter)) { 
            // Print the current quote and nextCharacter as is
            writeCharacter << currentCharacter;
            writeCharacter << nextCharacter;
            
            // Store the type of quote we are processing
            QChar quote = currentCharacter;

            // Don't modify the quoted strings
            while (nextCharacter != quote) {
                readCharacter >> nextCharacter;
                writeCharacter << nextCharacter;
            }

            //Start fresh after handling quoted strings
            previousCharacter = nextCharacter;
            readCharacter >> currentCharacter;
            continue;
        } else {
            // In all other cases write the currentCharacter to outputFile
            writeCharacter << currentCharacter;  
        } 

        previousCharacter = currentCharacter;
        currentCharacter = nextCharacter;
    }
    
    //write currentCharacter to output file when nextCharacter reaches EOF
    if (currentCharacter != '\n') {
        writeCharacter << currentCharacter;
    }
    
    // Reading done. Closing the inputFile
    inputFile->close();

    if (hasErrors()) {
        return;
    } else {
        // Bake successful, Export the compressed outputFile
        exportJS(&outputFile);
    }
}

void JSBaker::exportJS(QFile* bakedFile) {
    // save the relative path to this JS inside the output folder
    auto fileName = _jsURL.fileName();
    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    auto bakedFilename = baseName + BAKED_JS_EXTENSION;

    _bakedJSFilePath = _bakedOutputDir + "/" + bakedFilename;

    bakedFile->setFileName(_bakedJSFilePath);

    if (!bakedFile->open(QIODevice::WriteOnly)) {
        handleError("Error opening " + _bakedJSFilePath + " for writing");
    }

    if (hasErrors()) {
        // cannot open bakedFile return
        return;
    } else {
        _outputFiles.push_back(_bakedJSFilePath);
        qCDebug(js_baking) << "Exported" << _jsURL << "with re-written paths to" << _bakedJSFilePath;
    }

    // Exporting done. Closing the outputFile
    bakedFile->close();
    // emit signal to indicate the JS baking is finished
    emit finished();
}

void JSBaker::handleSingleLineComments(QTextStream * readCharacter) {
    QChar character;
    while (!readCharacter->atEnd()) {
        *readCharacter >> character;
        if (character == '\n')
        break;
    }
    return;
}

void JSBaker::handleMultiLineComments(QTextStream * readCharacter) {
    QChar character;
    while (!readCharacter->atEnd()) {
        *readCharacter >> character;
        if (character == '*') {
            if (readCharacter->read(1) == '/') {
                return;
            }
        }
    }
    handleError("Eror unterminated multi line comment");
}

bool JSBaker::canOmitSpace(QChar previousCharacter, QChar nextCharacter) {
    if ((isAlphanum(previousCharacter) || isNonAscii(previousCharacter) || isSpecialCharacter(previousCharacter)) &&
        (isAlphanum(nextCharacter) || isNonAscii(nextCharacter) || isSpecialCharacter(nextCharacter))
       ) {
        return false;
    }
    return true;
}

bool JSBaker::canOmitNewLine(QChar previousCharacter, QChar nextCharacter) {
    if ((isAlphanum(previousCharacter) || isNonAscii(previousCharacter) || isSpecialCharacterPrevious(previousCharacter)) &&
        (isAlphanum(nextCharacter) || isNonAscii(nextCharacter) || isSpecialCharacterNext(nextCharacter))
       ) {
        return false;
    }
    return true;
}

//Check if character is alphabet, number or one of the following: '_', '$', '\\' 
bool JSBaker::isAlphanum(QChar c) {
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') 
            || c == '_' || c == '$' || c == '\\' || c > 126);
}

bool JSBaker::isNonAscii(QChar c) {
    return ((int)c.toLatin1() > 127);
}

bool JSBaker::isSpecialCharacter(QChar c) {
    if (c == '\'' || c == '$' || c == '_' || c == '/') {
        return true;
    }
    return false;
}

bool JSBaker::isSpecialCharacterPrevious(QChar c) {
    if (c == '\'' || c == '$' || c == '_' || c == '}' || c == ']' || c == ')' || c == '+' || c == '-'
        || c == '"' || c == "'") {
        return true;
    }
    return false;
}

bool JSBaker::isSpecialCharacterNext(QChar c) {
    if (c == '\'' || c == '$' || c == '_' || c == '{' || c == '[' || c == '(' || c == '+' || c == '-') {
        return true;
    }
    return false;
}

// Check if white space or tab
bool JSBaker::isControlCharacter(QChar c) {
    return (c == ' ' || (int)c.toLatin1() == 9);
}

// Check If the currentCharacter is " or ' or `
bool JSBaker::isQuote(QChar c) {
    return (c == '"' || c == "'" || c == '`');
}
