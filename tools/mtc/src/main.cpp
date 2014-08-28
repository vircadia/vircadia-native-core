//
//  main.cpp
//  tools/mtc/src
//
//  Created by Andrzej Kapolka on 12/31/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>

#include <QFile>
#include <QList>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>

using namespace std;

class Class {
public:
    QString name;
    QStringList bases;
};

class Field {
public:
    QString type;
    QString name;
};

class Streamable {
public:
    Class clazz;
    QList<Field> fields;
};

void processInput(QTextStream& in, QList<Streamable>* streamables) {
    Class clazz;
    Streamable currentStreamable;

    QRegExp exp(
        "(/\\*.*\\*/)|" // multi-line comments
        "(//.*\n)|" // single-line comments
        "(\\s*#.*\n)|" // preprocessor definitions
        "(\\s*STREAMABLE\\s+)|" // STREAMABLE tag for classes
        "(\\s*STREAM\\s+.*;)|" // STREAM tag for fields
        "(\\s*class\\s+[^;]+\\{)" // class definition
    );
    exp.setMinimal(true);

    QRegExp classExp("class (\\w+) ?:?([^:]*)\\{");

    // read in the entire input and look for matches with our expression
    QString all = in.readAll();
    for (int off = 0; (off = exp.indexIn(all, off)) != -1; off += exp.matchedLength()) {
        QString match = exp.cap().simplified();
        if (match.startsWith("/*") || match.startsWith("//") || match.startsWith('#')) {
            continue; // comment, preprocessor definition
        }
        if (match.startsWith("STREAMABLE")) {
            if (clazz.name.isEmpty()) {
                cerr << "Found STREAMABLE marker before class definition." << endl;
                continue;
            }
            if (!currentStreamable.clazz.name.isEmpty()) {
                streamables->append(currentStreamable);
            }
            currentStreamable.clazz = clazz;
            currentStreamable.fields.clear();

        } else if (match.startsWith("STREAM")) {
            match.chop(1); // get rid of the semicolon
            match = match.mid(match.indexOf(' ') + 1).trimmed(); // and STREAM, and any space before it
            int index = match.lastIndexOf(' ');
            Field field = { match.left(index).simplified(), match.mid(index + 1) };
            currentStreamable.fields.append(field);

        } else { // match.startsWith("class")
            classExp.exactMatch(match);
            clazz.name = classExp.cap(1);
            clazz.bases.clear();
            foreach (const QString& bstr, classExp.cap(2).split(',')) {
                QString base = bstr.trimmed();
                if (!base.isEmpty() && base.startsWith("STREAM")) {
                    clazz.bases.append(base.mid(base.lastIndexOf(' ') + 1));
                }
            }
        }
    }
    if (!currentStreamable.clazz.name.isEmpty()) {
        streamables->append(currentStreamable);
    }
}

void generateOutput (QTextStream& out, const QList<Streamable>& streamables) {
    foreach (const Streamable& str, streamables) {
        const QString& name = str.clazz.name;

        out << "const int " << name << "::Type = registerStreamableMetaType<" << name << ">();\n";

        out << "const QVector<MetaField>& " << name << "::getMetaFields() {\n";
        out << "    static QVector<MetaField> metaFields = QVector<MetaField>()";
        foreach (const QString& base, str.clazz.bases) {
            out << " << " << base << "::getMetaFields()";
        }
        foreach (const Field& field, str.fields) {
            out << "\n        << MetaField(\"" << field.name << "\", Bitstream::getTypeStreamer(qMetaTypeId<" <<
                field.type << ">()))";
        }
        out << ";\n";
        out << "    return metaFields;\n";
        out << "}\n";

        out << "int " << name << "::getFieldIndex(const QByteArray& name) {\n";
        out << "    static QHash<QByteArray, int> fieldIndices = createFieldIndices();\n";
        out << "    return fieldIndices.value(name) - 1;\n";
        out << "}\n";

        out << "QHash<QByteArray, int> " << name << "::createFieldIndices() {\n";
        out << "    QHash<QByteArray, int> indices;\n";
        out << "    int index = 1;\n";
        out << "    foreach (const MetaField& field, getMetaFields()) {\n";
        out << "        indices.insert(field.getName(), index++);\n";
        out << "    }\n";
        out << "    return indices;\n";
        out << "}\n";

        out << "void " << name << "::setField(int index, const QVariant& value) {\n";
        if (!str.clazz.bases.isEmpty()) {
            out << "    int nextIndex;\n";
        }
        foreach (const QString& base, str.clazz.bases) {
            out << "    if ((nextIndex = index - " << base << "::getMetaFields().size()) < 0) {\n";
            out << "        " << base << "::setField(index, value);\n";
            out << "        return;\n";
            out << "    }\n";
            out << "    index = nextIndex;\n";        
        }
        if (!str.fields.isEmpty()) {
            out << "    switch (index) {\n";
            for (int i = 0; i < str.fields.size(); i++) {
                out << "        case " << i << ":\n";
                out << "            this->" << str.fields.at(i).name << " = value.value<" << str.fields.at(i).type << ">();\n";
                out << "            break;\n";
            }
            out << "    }\n";
        }
        out << "}\n";

        out << "QVariant " << name << "::getField(int index) const {\n";
        if (!str.clazz.bases.isEmpty()) {
            out << "    int nextIndex;\n";
        }
        foreach (const QString& base, str.clazz.bases) {
            out << "    if ((nextIndex = index - " << base << "::getMetaFields().size()) < 0) {\n";
            out << "        return " << base << "::getField(index);\n";
            out << "    }\n";
            out << "    index = nextIndex;\n";        
        }
        if (!str.fields.isEmpty()) {
            out << "    switch (index) {\n";
            for (int i = 0; i < str.fields.size(); i++) {
                out << "        case " << i << ":\n";
                out << "            return QVariant::fromValue(this->" << str.fields.at(i).name << ");\n";
            }
            out << "    }\n";
        }
        out << "    return QVariant();\n";
        out << "}\n";
        
        out << "Bitstream& operator<<(Bitstream& out, const " << name << "& obj) {\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    out << static_cast<const " << base << "&>(obj);\n";
        }
        foreach (const Field& field, str.fields) {
            out << "    out << obj." << field.name << ";\n";
        }
        out << "    return out;\n";
        out << "}\n";

        out << "Bitstream& operator>>(Bitstream& in, " << name << "& obj) {\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    in >> static_cast<" << base << "&>(obj);\n";
        }
        foreach (const Field& field, str.fields) {
            out << "    in >> obj." << field.name << ";\n";
        }
        out << "    return in;\n";
        out << "}\n";

        out << "template<> void Bitstream::writeRawDelta(const " << name << "& value, const " << name << "& reference) {\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    writeRawDelta(static_cast<const " << base << "&>(value), static_cast<const " <<
                base << "&>(reference));\n";
        }
        foreach (const Field& field, str.fields) {
            out << "    writeDelta(value." << field.name << ", reference." << field.name << ");\n";
        }
        out << "}\n";

        out << "template<> void Bitstream::readRawDelta(" << name << "& value, const " << name << "& reference) {\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    readRawDelta(static_cast<" << base << "&>(value), static_cast<const " <<
                base << "&>(reference));\n";
        }
        foreach (const Field& field, str.fields) {
            out << "    readDelta(value." << field.name << ", reference." << field.name << ");\n";
        }
        out << "}\n";
        
        out << "template<> QJsonValue JSONWriter::getData(const " << name << "& value) {\n";
        out << "    QJsonArray array;\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    foreach (const QJsonValue& element, getData(static_cast<const " << base << "&>(value)).toArray()) {\n";
            out << "        array.append(element);\n";
            out << "    }\n";
        }
        foreach (const Field& field, str.fields) {
            out << "    array.append(getData(value." << field.name << "));\n";
        }
        out << "    return array;\n";
        out << "}\n";
        
        out << "template<> void JSONReader::putData(const QJsonValue& data, " << name << "& value) {\n";
        if (!(str.clazz.bases.isEmpty() && str.fields.isEmpty())) {
            out << "    QJsonArray array = data.toArray(), subarray;\n";
            out << "    QJsonArray::const_iterator it = array.constBegin();\n";
            foreach (const QString& base, str.clazz.bases) {
                out << "    subarray = QJsonArray();\n";
                out << "    for (int i = 0; i < " << base << "::getMetaFields().size(); i++) {\n";
                out << "        subarray.append(*it++);\n";
                out << "    }\n";
                out << "    putData(subarray, static_cast<" << base << "&>(value));\n";
            }
            foreach (const Field& field, str.fields) {
                out << "    putData(*it++, value." << field.name << ");\n";
            }
        }
        out << "}\n";
        
        out << "bool operator==(const " << name << "& first, const " << name << "& second) {\n";
        if (str.clazz.bases.isEmpty() && str.fields.isEmpty()) {
            out << "    return true";   
        } else {
            out << "    return ";
            bool first = true;
            foreach (const QString& base, str.clazz.bases) {
                if (!first) {
                    out << " &&\n";
                    out << "        ";
                }
                out << "static_cast<const " << base << "&>(first) == static_cast<const " << base << "&>(second)";
                first = false;
            }
            foreach (const Field& field, str.fields) {
                if (!first) {
                    out << " &&\n";
                    out << "        ";
                }
                out << "first." << field.name << " == second." << field.name;    
                first = false;
            }
        }
        out << ";\n";
        out << "}\n";

        out << "bool operator!=(const " << name << "& first, const " << name << "& second) {\n";
        if (str.clazz.bases.isEmpty() && str.fields.isEmpty()) {
            out << "    return false";   
        } else {
            out << "    return ";
            bool first = true;
            foreach (const QString& base, str.clazz.bases) {
                if (!first) {
                    out << " ||\n";
                    out << "        ";
                }
                out << "static_cast<const " << base << "&>(first) != static_cast<const " << base << "&>(second)";
                first = false;
            }
            foreach (const Field& field, str.fields) {
                if (!first) {
                    out << " ||\n";
                    out << "        ";
                }
                out << "first." << field.name << " != second." << field.name;    
                first = false;
            }
        }
        out << ";\n";
        out << "}\n\n";
    }
}

int main (int argc, char** argv) {
    // process the command line arguments
    QStringList inputs;
    QString output;
    for (int ii = 1; ii < argc; ii++) {
        QString arg(argv[ii]);
        if (!arg.startsWith('-')) {
            inputs.append(arg);
            continue;
        }
        QStringRef name = arg.midRef(1);
        if (name == "o") {
            if (++ii == argc) {
                cerr << "Missing file name argument for -o" << endl;
                return 1;
            }
            output = argv[ii];

        } else {
            cerr << "Unknown option " << arg.toStdString() << endl;
            return 1;
        }
    }
    if (inputs.isEmpty()) {
        cerr << "Usage: mtc [OPTION]... input files" << endl;
        cerr << "Where options include:" << endl;
        cerr << "  -o filename: Send output to filename rather than standard output." << endl;
        return 0;
    }

    QList<Streamable> streamables;
    foreach (const QString& input, inputs) {
        QFile ifile(input);
        if (!ifile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            cerr << ("Couldn't open " + input + ": " + ifile.errorString()).toStdString() << endl;
            continue;
        }
        QTextStream istream(&ifile);
        int oldSize = streamables.size();
        processInput(istream, &streamables);
        if (streamables.size() == oldSize) {
            // no streamables; remove from list
            inputs.removeOne(input);
        }
    }

    QFile ofile(output);
    if (output.isNull()) {
        ofile.open(stdout, QIODevice::WriteOnly | QIODevice::Text);

    } else if (!ofile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        cerr << ("Couldn't open " + output + ": " + ofile.errorString()).toStdString() << endl;
        return 1;
    }

    QTextStream ostream(&ofile);
    ostream << "// generated by mtc\n";
    foreach (const QString& input, inputs) {
        ostream << "#include \"" << input << "\"\n";
    }
    generateOutput(ostream, streamables);

    return 0;
}
