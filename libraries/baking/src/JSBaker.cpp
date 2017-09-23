#include <QtConcurrent>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <PathUtils.h>

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
    qDebug() << "Made temporary dir " << _tempDir;
    qDebug() << "Origin file path: " << _originalJSFilePath;

    connect(this, &JSBaker::sourceCopyReadyToLoad, this, &JSBaker::bakeSourceCopy);

    loadSourceJS();
}

void JSBaker::loadSourceJS() {
    
    // load the local file
    QFile localJS{ _jsURL.toLocalFile() };
    
    if (!localJS.exists()) {
        
        handleError("Could not find " + _jsURL.toString());
        return;
    }

    // copy to original file
    qDebug() << "Local file url: " << _jsURL << _jsURL.toString() << _jsURL.toLocalFile() << ", copying to: " << _originalJSFilePath;
    localJS.copy(_originalJSFilePath);

    // emit signal to indicate script is ready to import
    emit sourceCopyReadyToLoad();
}

void JSBaker::bakeSourceCopy() {
    
    importScript();

    if (hasErrors()) {
        return;
    }

    /*bakeScript();
    if (hasErrors()) {
        return;
    }*/
    
    /*exportScript();

    if (hasErrors()) {
        return;
    }*/
}



void JSBaker::importScript() {
    
    //qDebug() << "file path: " << _originalJSFilePath.toLocal8Bit().data() << QDir(_originalJSFilePath).exists();
    
    // Import the file to be processed
    QFile jsFile(_originalJSFilePath);
    if (!jsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        handleError("Error opening " + _originalJSFilePath + " for reading");
        return;
    }

    // Call Bake Script with the opened file
    bakeScript(&jsFile);
}

void JSBaker::bakeScript(QFile* inFile) {

    QFile outFile;
    outFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream in(inFile);
    QTextStream out(&outFile);

    QChar currentChar;
    QChar prevChar;
    QChar nextChar;
    
    // Initialize prevChar with new line
    prevChar = '\n';
    in >> currentChar;
    
    while (!in.atEnd()) {
        
        // Reading per character
        in >> nextChar;

        if (currentChar == '\r') {
            out << '\n';
            //currentChar = '\n';
            
        } else if (currentChar == '/') {
            
            if (nextChar == '/') {
                 
                handleSingleLineComments(&in);
                //out << '\n';
                
                //Start fresh after handling comments
                prevChar = '\n';
                in >> currentChar;
                continue;

            } else if (nextChar == '*') {
                
                handleMultiLineComments(&in);
                //out << ' ';
                
                //Start fresh after handling comments
                prevChar = '\n';
                in >> currentChar;
                //currentChar = ' ';
                continue;
            } else {
                out << currentChar;
            }
        } else if (currentChar == ' ' || (int) currentChar.toLatin1() == 9) {
            
            // Handle multiple spaces
            if (nextChar == ' ' || (int)currentChar.toLatin1() == 9) {
                while (nextChar == ' ' || (int)nextChar.toLatin1() == 9 ) {
                    
                    in >> nextChar;
                    if (nextChar == '\n')
                        break;
                }
            }
            if (!omitSpace(prevChar,nextChar)) {

                out << ' ';
            }

        } else if (currentChar == '\n') {
            qDebug() << "prevChar2" << prevChar;
            qDebug() << "currentChar2" << currentChar;
            qDebug() << "nextChar2" << nextChar;
            //Handle multiple new lines
            //Hnadle new line followed by space or tab
            if (nextChar == '\n' || nextChar == ' ' || (int)nextChar.toLatin1() == 9) {
                while (nextChar == '\n' || nextChar == ' ' || (int)nextChar.toLatin1() == 9) {
                    in >> nextChar;
                }
            } 

            if (!omitNewLine(prevChar, nextChar)) {
                out << '\n';
            }
            
        } else if (currentChar == '"' ) {
            //Don't modify quoted strings
            out << currentChar;
            out << nextChar;
            while (nextChar != '"') {
                in >> nextChar;
                out << nextChar;
            }

            
            //Start fresh after handling quoted strings
            //prevChar = '"';
            prevChar = nextChar;
            //currentChar = nextChar;
            in >> currentChar;
            continue;
        } else if (currentChar == "'") {
            //Don't modify quoted strings
            out << currentChar;
            out << nextChar;
            while (nextChar != "'") {
                in >> nextChar;
                out << nextChar;
            }
            
            qDebug() << "prevChar" << prevChar;
            qDebug() << "currentChar" << currentChar;
            qDebug() << "nextChar" << nextChar;

            //out << nextChar;
            //Start fresh after handling quoted strings
            //prevChar = '\'';
            prevChar = nextChar;
            //currentChar = nextChar;
            in >> currentChar;
            qDebug() << "prevChar1" << prevChar;
            qDebug() << "currentChar1" << currentChar;
            qDebug() << "nextChar1" << nextChar;
            continue;
        } else {
            out << currentChar;
            
        } 

        prevChar = currentChar;
        currentChar = nextChar;
    }
    
    //Output current character when next character reaches EOF
    if (currentChar != '\n') {

        out << currentChar;
    }
    
    
    inFile->close();
    exportScript(&outFile);
}

void JSBaker::handleSingleLineComments(QTextStream * in) {
    QChar ch;
    
    while (!in->atEnd()) {
        *in >> ch;
        if (ch <= '\n')
        break;
    }
    
    //*out << '\n';
}

void JSBaker::handleMultiLineComments(QTextStream * in) {
    QChar ch;

    while (!in->atEnd()) {
        *in >> ch;
        if (ch == '*') {
            if (in->read(1) == '/') {
                return;
            }
        }
    }

    handleError("Eror unterminated multi line comment");

}

bool JSBaker::isAlphanum(QChar c) {
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
            c > 126);
}

bool JSBaker::omitSpace(QChar prevChar, QChar nextChar) {

    if ((isAlphanum(prevChar) || isNonAscii(prevChar) || isSpecialChar(prevChar)) &&
        (isAlphanum(nextChar) || isNonAscii(nextChar) || isSpecialChar(nextChar))
        ) {

        return false;
    }

    return true;

}

bool JSBaker::isNonAscii(QChar c) {
    return ((int)c.toLatin1() > 127);
}

bool JSBaker::isSpecialChar(QChar c) {
    
    if (c == '\'' || c == '$' || c == '_')
        return true;

    return false;
}

bool JSBaker::isSpecialCharPre(QChar c) {

    if (c == '\'' || c == '$' || c == '_' || c == '}' || c == ']' || c == ')' || c == '+' || c == '-' 
        || c == '"' || c == "'")
        return true;

    return false;
}

bool JSBaker::isSpecialCharPost(QChar c) {

    if (c == '\'' || c == '$' || c == '_' || c == '{' || c == '[' || c == '(' || c == '+' || c == '-' || c == '/')
        return true;

    return false;
}

bool JSBaker::omitNewLine(QChar prevChar, QChar nextChar) {

    if ((isAlphanum(prevChar) || isNonAscii(prevChar) || isSpecialCharPre(prevChar)) &&
        (isAlphanum(nextChar) || isNonAscii(nextChar) || isSpecialCharPost(nextChar))
        ) {

        return false;
    }

    return true;

}

void JSBaker::exportScript(QFile* bakedFile) {
    // save the relative path to this FBX inside our passed output folder
    auto fileName = _jsURL.fileName();
    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    auto bakedFilename = baseName + BAKED_JS_EXTENSION;

    _bakedJSFilePath = _bakedOutputDir + "/" + bakedFilename;

    bakedFile->setFileName(_bakedJSFilePath);

    //QFile bakedFile(_bakedJSFilePath);

    if (!bakedFile->open(QIODevice::WriteOnly)) {
        handleError("Error opening " + _bakedJSFilePath + " for writing");
        return;
    }

    /*QByteArray x(5, 'a');
    bakedFile.copy();*/

    _outputFiles.push_back(_bakedJSFilePath);

    qCDebug(model_baking) << "Exported" << _jsURL << "with re-written paths to" << _bakedJSFilePath;
    bakedFile->close();
    emit finished();
}