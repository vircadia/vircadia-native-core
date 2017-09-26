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

#include <PathUtils.h>

#include "JSBaker.h"
#include "Baker.h"

const int ASCII_CHARACTERS_UPPER_LIMIT = 126;

JSBaker::JSBaker(const QUrl& jsURL, const QString& bakedOutputDir) :
    _jsURL(jsURL),
    _bakedOutputDir(bakedOutputDir)
{
    
};

void JSBaker::bake() {
    qCDebug(js_baking) << "JS Baker " << _jsURL << "bake starting";
    
    // Import the script to start baking
    importJS();
}

void JSBaker::importJS() {    
    // Import the file to be processed from _jsURL
    QFile jsFile(_jsURL.toLocalFile());
    if (!jsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        handleError("Error opening " + _originalJSFilePath + " for reading");
        return;
    }
    
    // Import successful, Call the baking function with the imported file
    bakeJS(&jsFile);    
}

void JSBaker::bakeJS(QFile* inputFile) {
    // Create an output file which will be exported as the bakedFile
    QFile outputFile;
    outputFile.open(QIODevice::WriteOnly | QIODevice::Text);
    
    // Read from inputFile and write to outputFile per character 
    QTextStream in(inputFile);
    QTextStream out(&outputFile);

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
                handleSingleLineComments(&in);
                
                //Start fresh after handling comments
                previousCharacter = '\n';
                in >> currentCharacter;
                continue;
            } else if (nextCharacter == '*') {
                // Check if multi line comment i.e. /*
                handleMultiLineComments(&in);
                
                //Start fresh after handling comments
                previousCharacter = '\n';
                in >> currentCharacter;
                continue;
            } else {
                // If '/' is not followed by '/' or '*' print '/'
                out << currentCharacter;
            }
        } else if (isControlCharacter(currentCharacter)) { 
            // Check if white space or tab

            // Skip multiple spaces or tabs
            if (isControlCharacter(nextCharacter)) {
                while (isControlCharacter(nextCharacter)) {
                    in >> nextCharacter;
                    if (nextCharacter == '\n') {
                        break;
                    }
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
            while (nextCharacter == '\n' || isControlCharacter(nextCharacter)) {
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
        return;
    }
    
    // Export successful
    _outputFiles.push_back(_bakedJSFilePath);
    qCDebug(js_baking) << "Exported" << _jsURL << "with re-written paths to" << _bakedJSFilePath;
    
    // Exporting done. Closing the outputFile
    bakedFile->close();
    
    // emit signal to indicate the JS baking is finished
    emit finished();
}

void JSBaker::handleSingleLineComments(QTextStream * in) {
    QChar character;
    while (!in->atEnd()) {
        *in >> character;
        if (character == '\n') {
            break;
        }
    }
}

void JSBaker::handleMultiLineComments(QTextStream * in) {
    QChar character;
    while (!in->atEnd()) {
        *in >> character;
        if (character == '*') {
            if (in->read(1) == '/') {
                return;
            }
        }
    }
    handleError("Eror unterminated multi line comment");
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
    return (c == '\'' || c == '$' || c == '_' || c == '/');
}

// If previous character is a special character, maybe don't omit new line (depends on next character as well)
bool JSBaker::isSpecialCharacterPrevious(QChar c) {
    return (c == '\'' || c == '$' || c == '_' || c == '}' || c == ']' || c == ')' || c == '+' || c == '-'
            || c == '"' || c == "'");
}

// If next character is a special character, maybe don't omit new line (depends on previous character as well)
bool JSBaker::isSpecialCharacterNext(QChar c) {
    return (c == '\'' || c == '$' || c == '_' || c == '{' || c == '[' || c == '(' || c == '+' || c == '-');
}

// Check if white space or tab
bool JSBaker::isControlCharacter(QChar c) {
    return (c == ' ' || c == '\t');
}

// Check If the currentCharacter is " or ' or `
bool JSBaker::isQuote(QChar c) {
    return (c == '"' || c == "'" || c == '`');
}
