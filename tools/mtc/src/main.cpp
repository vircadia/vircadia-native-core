//
//  main.cpp
//  mtc
//
//  Created by Andrzej Kapolka on 12/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

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

class Streamable {
public:
    Class clazz;
    QStringList fields;
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
            match = match.trimmed(); // and any space before it
            currentStreamable.fields.append(match.mid(match.lastIndexOf(' ') + 1));

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

        out << "Bitstream& operator<<(Bitstream& out, const " << name << "& obj) {\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    out << static_cast<const " << base << "&>(obj);\n";
        }
        foreach (const QString& field, str.fields) {
            out << "    out << obj." << field << ";\n";
        }
        out << "    return out;\n";
        out << "}\n";

        out << "Bitstream& operator>>(Bitstream& in, " << name << "& obj) {\n";
        foreach (const QString& base, str.clazz.bases) {
            out << "    in >> static_cast<" << base << "&>(obj);\n";
        }
        foreach (const QString& field, str.fields) {
            out << "    in >> obj." << field << ";\n";
        }
        out << "    return in;\n";
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
            foreach (const QString& field, str.fields) {
                if (!first) {
                    out << " &&\n";
                    out << "        ";
                }
                out << "first." << field << " == second." << field;    
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
            foreach (const QString& field, str.fields) {
                if (!first) {
                    out << " ||\n";
                    out << "        ";
                }
                out << "first." << field << " != second." << field;    
                first = false;
            }
        }
        out << ";\n";
        out << "}\n";
        
        out << "const int " << name << "::Type = registerStreamableMetaType<" << name << ">();\n\n";
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
