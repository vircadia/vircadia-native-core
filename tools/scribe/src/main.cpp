//
//  main.cpp
//  tools/shaderScribe/src
//
//  Created by Sam Gateau on 12/15/2014.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


#include "TextTemplate.h"

#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>

using namespace std;

int main (int argc, char** argv) {
    // process the command line arguments
    std::vector< std::string > inputs;

    std::string srcFilename;
    std::string destFilename;
    std::string targetName;
    TextTemplate::Vars vars;
    
    std::string lastVarName;
    bool listVars = false;
    bool showParseTree = false;
    bool makeCPlusPlus = false;

    auto config = std::make_shared<TextTemplate::Config>();

    enum Mode {
        READY = 0,
        GRAB_OUTPUT,
        GRAB_VAR_NAME,
        GRAB_VAR_VALUE,
        GRAB_INCLUDE_PATH,
        GRAB_TARGET_NAME,
        EXIT,
    } mode = READY;

    for (int ii = 1; (mode != EXIT) && (ii < argc); ii++) {
        inputs.push_back(argv[ii]);

        switch (mode) {
            case READY: {
                if (inputs.back() == "-o") {
                    mode = GRAB_OUTPUT;
                } else if (inputs.back() == "-t") {
                    mode = GRAB_TARGET_NAME;
                } else if (inputs.back() == "-D") {
                    mode = GRAB_VAR_NAME;
                } else if (inputs.back() == "-I") {
                    mode = GRAB_INCLUDE_PATH;
                } else if (inputs.back() == "-listVars") {
                    listVars = true;
                    mode = READY;
                } else if (inputs.back() == "-showParseTree") {
                    showParseTree = true;
                    mode = READY;
                } else if (inputs.back() == "-c++") {
                    makeCPlusPlus = true;
                    mode = READY;
                } else {
                    // just grabbed the source filename, stop parameter parsing
                    srcFilename = inputs.back();
                    mode = EXIT;
                }
            }
            break;

            case GRAB_OUTPUT: {
                destFilename = inputs.back();
                mode = READY;
            }
            break;

            case GRAB_TARGET_NAME: {
                targetName = inputs.back();
                mode = READY;
            }
            break;

            case GRAB_VAR_NAME: {
                // grab first the name of the var
                lastVarName = inputs.back();
                mode = GRAB_VAR_VALUE;
            }
            break;
            case GRAB_VAR_VALUE: {
                // and then the value
                vars.insert(TextTemplate::Vars::value_type(lastVarName, inputs.back()));

                mode = READY;
            }
            break;

            case GRAB_INCLUDE_PATH: {
                config->addIncludePath(inputs.back().c_str());
                mode = READY;
            }
            break;
                
            case EXIT: {
                // THis shouldn't happen
            }
            break;
        }
    }

    if (srcFilename.empty()) {
        cerr << "Usage: shaderScribe [OPTION]... inputFilename" << endl;
        cerr << "Where options include:" << endl;
        cerr << "  -o filename: Send output to filename rather than standard output." << endl;
        cerr << "  -t targetName: Set the targetName used, if not defined use the output filename 'name' and if not defined use the inputFilename 'name'" << endl;
        cerr << "  -I include_directory: Declare a directory to be added to the includes search pool." << endl;
        cerr << "  -D varname varvalue: Declare a var used to generate the output file." << endl;
        cerr << "       varname and varvalue must be made of alpha numerical characters with no spaces." << endl;
        cerr << "  -listVars : Will list the vars name and value in the standard output." << endl;
        cerr << "  -showParseTree : Draw the tree obtained while parsing the source" << endl;
        cerr << "  -c++ : Generate a c++ header file containing the output file stream stored as a char[] variable" << endl;
        return 0;
    }

    // Define targetName: if none, get destFilenmae, if none get srcFilename
    if (targetName.empty()) {
        if (destFilename.empty()) {
            targetName = srcFilename;
        } else {
            targetName = destFilename;
        }
    }
    // no clean it to have just a descent c var name
    if (!targetName.empty()) {
        // trim anything before '/' or '\'
        targetName = targetName.substr(targetName.find_last_of('/') + 1);
        targetName = targetName.substr(targetName.find_last_of('\\') + 1);

        // trim anything after '.'
        targetName = targetName.substr(0, targetName.find_first_of('.'));
    }

    // Add built in vars
    time_t endTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    std::string endTimStr(ctime(&endTime));
    vars["_SCRIBE_DATE"] = endTimStr.substr(0, endTimStr.length() - 1);

    // List vars?
    if (listVars) {
        cerr << "Vars:" << endl;
        for (auto v : vars) {
            cerr << "    " << v.first << " = " << v.second << endl;
        }
    }

    // Open up source
    std::fstream srcStream;
    srcStream.open(srcFilename, std::fstream::in);
    if (!srcStream.is_open()) {
        cerr << "Failed to open source file <" << srcFilename << ">" << endl;
        return 0;
    }

    auto scribe = std::make_shared<TextTemplate>(srcFilename, config);

    // ready to parse and generate
    std::stringstream destStringStream;
    int numErrors = scribe->scribe(destStringStream, srcStream, vars);
    if (numErrors) {
        cerr << "Scribe " << srcFilename << "> failed: " << numErrors << " errors." << endl;
        return 0;
    };


    if (showParseTree) {
        int level = 1;
        cerr << "Config trees:" << std::endl;
        config->displayTree(cerr, level);

        cerr << srcFilename << " tree:" << std::endl;
        scribe->displayTree(cerr, level);
    }

    std::ostringstream targetStringStream;
    if (makeCPlusPlus) {
        // Because there is a maximum size for literal strings declared in source we need to partition the 
        // full source string stream into pages that seems to be around that value...
        const int MAX_STRING_LITERAL = 10000;
        std::string lineToken;
        auto pageSize = lineToken.length();
        std::vector<std::shared_ptr<std::stringstream>> pages(1, std::make_shared<std::stringstream>());
        while (!destStringStream.eof()) {
            std::getline(destStringStream, lineToken);
            auto lineSize = lineToken.length() + 1;

            if (pageSize + lineSize > MAX_STRING_LITERAL) {
                pages.push_back(std::make_shared<std::stringstream>());
                // reset pageStringStream
                pageSize = 0;
            }

            (*pages.back()) << lineToken << std::endl;
            pageSize += lineSize;
        }

        targetStringStream << "// File generated by Scribe " << vars["_SCRIBE_DATE"] << std::endl;
        targetStringStream << "#ifndef scribe_" << targetName << "_h" << std::endl;
        targetStringStream << "#define scribe_" << targetName << "_h" << std::endl << std::endl;

        targetStringStream << "const char " << targetName << "[] = \n";

        // Write the pages content
        for (auto page : pages) {
            targetStringStream << "R\"SCRIBE(\n" << page->str() << "\n)SCRIBE\"\n";
        }
        targetStringStream << ";\n" << std::endl << std::endl;

        targetStringStream << "#endif" << std::endl;
    } else {
        targetStringStream << destStringStream.str();
    }

    // Destination stream
    if (!destFilename.empty()) {
        std::fstream destFileStream;
        destFileStream.open(destFilename, std::fstream::out);
        if (!destFileStream.is_open()) {
            cerr << "Scribe output file " << destFilename << "> failed to open." << endl;
            return 0;
        }
        
        destFileStream << targetStringStream.str();
    } else {
        cerr << targetStringStream.str();
    }

    return 0;

}
