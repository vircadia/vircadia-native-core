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
    std::list<std::string> headerFiles;
    TextTemplate::Vars vars;
    
    std::string lastVarName;
    bool listVars = false;
    bool makefileDeps = false;
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
        GRAB_SHADER_TYPE,
        GRAB_HEADER,
        EXIT,
    } mode = READY;

    enum Type {
        VERTEX = 0,
        FRAGMENT,
        GEOMETRY,
    } type = VERTEX;
    static const char* shaderTypeString[] = {
        "VERTEX", "PIXEL", "GEOMETRY"
    };
    static const char* shaderCreateString[] = {
        "Vertex", "Pixel", "Geometry"
    };
    std::string shaderStage{ "vert" };

    for (int ii = 1; (mode != EXIT) && (ii < argc); ii++) {
        inputs.push_back(argv[ii]);

        switch (mode) {
            case READY: {
                if (inputs.back() == "-o") {
                    mode = GRAB_OUTPUT;
                } else if (inputs.back() == "-H") {
                    mode = GRAB_HEADER;
                } else if (inputs.back() == "-t") {
                    mode = GRAB_TARGET_NAME;
                } else if (inputs.back() == "-D") {
                    mode = GRAB_VAR_NAME;
                } else if (inputs.back() == "-I") {
                    mode = GRAB_INCLUDE_PATH;
                } else if (inputs.back() == "-listVars") {
                    listVars = true;
                    mode = READY;
                } else if (inputs.back() == "-M") {
                    makefileDeps = true;
                    mode = READY;
                } else if (inputs.back() == "-showParseTree") {
                    showParseTree = true;
                    mode = READY;
                } else if (inputs.back() == "-c++") {
                    makeCPlusPlus = true;
                    mode = READY;
                } else if (inputs.back() == "-T") {
                    mode = GRAB_SHADER_TYPE;
                } else {
                    // just grabbed the source filename, stop parameter parsing
                    srcFilename = inputs.back();
                    mode = READY;
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

            case GRAB_HEADER: {
                headerFiles.push_back(inputs.back());
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
                
            case GRAB_SHADER_TYPE:
            {
                if (inputs.back() == "frag") {
                    shaderStage = inputs.back();
                    type = FRAGMENT;
                } else if (inputs.back() == "geom") {
                    shaderStage = inputs.back();
                    type = GEOMETRY;
                } else if (inputs.back() == "vert") {
                    shaderStage = inputs.back();
                    type = VERTEX;
                } else {
                    cerr << "Unrecognized shader type. Supported is vert, frag or geom" << endl;
                }
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
        cerr << "  -H : Prepend the contents of header file to the scribe output " << endl;
        cerr << "       This can be specified multiple times and the headers will be applied in the specified order" << endl;
        cerr << "  -M : Emit a list of files that the scribe output depends on, for make and similar build tools " << endl;
        cerr << "  -listVars : Will list the vars name and value in the standard output." << endl;
        cerr << "  -showParseTree : Draw the tree obtained while parsing the source" << endl;
        cerr << "  -c++ : Generate a c++ source file containing the output file stream stored as a char[] variable" << endl;
        cerr << "  -T vert/frag/geom : define the type of the shader. Defaults to VERTEX if not specified." << endl;
        cerr << "       This is necessary if the -c++ option is used." << endl;
        return 0;
    }

    // Define targetName: if none, get destFilename, if none get srcFilename
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
        return 1;
    }

    auto scribe = std::make_shared<TextTemplate>(srcFilename, config);

    std::string header;
    if (!headerFiles.empty()) {
        for (const auto& headerFile : headerFiles) {
            std::fstream headerStream;
            headerStream.open(headerFile, std::fstream::in);
            if (!headerStream.is_open()) {
                cerr << "Failed to open source file <" << headerFile << ">" << endl;
                return 1;
            }
            header += std::string((std::istreambuf_iterator<char>(headerStream)), std::istreambuf_iterator<char>());
        }
    }

    // Add the type define to the shader
    switch (type) {
    case VERTEX:
        header += "#define GPU_VERTEX_SHADER\n";
        break;

    case FRAGMENT:
        header += "#define GPU_PIXEL_SHADER\n";
        break;

    case GEOMETRY:
        header += "#define GPU_GEOMETRY_SHADER\n";
        break;
    }

    // ready to parse and generate
    std::stringstream destStringStream;
    destStringStream << header;
    int numErrors = scribe->scribe(destStringStream, srcStream, vars);
    if (numErrors) {
        cerr << "Scribe " << srcFilename << "> failed: " << numErrors << " errors." << endl;
        return 1;
    };


    if (showParseTree) {
        int level = 1;
        cerr << "Config trees:" << std::endl;
        config->displayTree(cerr, level);

        cerr << srcFilename << " tree:" << std::endl;
        scribe->displayTree(cerr, level);
    }

    if (makefileDeps) {
        scribe->displayMakefileDeps(cout);
    } else if (makeCPlusPlus) {
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

        std::stringstream headerStringStream;
        std::stringstream sourceStringStream;
        std::string headerFileName = destFilename + ".h";
        std::string sourceFileName = destFilename + ".cpp";

        sourceStringStream << "// File generated by Scribe " << vars["_SCRIBE_DATE"] << std::endl;

        // Write header file
        headerStringStream << "#ifndef " << targetName << "_h" << std::endl;
        headerStringStream << "#define " << targetName << "_h\n" << std::endl;
        headerStringStream << "#include <gpu/Shader.h>\n" << std::endl;
        headerStringStream << "class " << targetName << " {" << std::endl;
        headerStringStream << "public:" << std::endl;
        headerStringStream << "\tstatic gpu::Shader::Type getType() { return gpu::Shader::" << shaderTypeString[type] << "; }" << std::endl;
        headerStringStream << "\tstatic const std::string& getSource() { return _source; }" << std::endl;
        headerStringStream << "\tstatic gpu::ShaderPointer getShader();" << std::endl;
        headerStringStream << "private:" << std::endl;
        headerStringStream << "\tstatic const std::string _source;" << std::endl;
        headerStringStream << "\tstatic gpu::ShaderPointer _shader;" << std::endl;
        headerStringStream << "};\n" << std::endl;
        headerStringStream << "#endif // " << targetName << "_h" << std::endl;

        bool mustOutputHeader = destFilename.empty();
        // Compare with existing file
        {
            std::fstream headerFile;
            headerFile.open(headerFileName, std::fstream::in);
            if (headerFile.is_open()) {
                // Skip first line
                std::string line;
                std::stringstream previousHeaderStringStream;
                std::getline(headerFile, line);

                previousHeaderStringStream << headerFile.rdbuf();
                mustOutputHeader = mustOutputHeader || previousHeaderStringStream.str() != headerStringStream.str();
            } else {
                mustOutputHeader = true;
            }
        }

        if (mustOutputHeader) {
            if (!destFilename.empty()) {
                // File content has changed so write it
                std::fstream headerFile;
                headerFile.open(headerFileName, std::fstream::out);
                if (headerFile.is_open()) {
                    // First line contains the date of modification
                    headerFile << sourceStringStream.str();
                    headerFile << headerStringStream.str();
                } else {
                    cerr << "Scribe output file <" << headerFileName << "> failed to open." << endl;
                    return 1;
                }
            } else {
                cerr << sourceStringStream.str();
                cerr << headerStringStream.str();
            }
        }

        // Write source file
        sourceStringStream << "#include \"" << targetName << ".h\"\n" << std::endl;
        sourceStringStream << "gpu::ShaderPointer " << targetName << "::_shader;" << std::endl;
        sourceStringStream << "const std::string " << targetName << "::_source = std::string()";
        // Write the pages content
        for (auto page : pages) {
            sourceStringStream << "+ std::string(R\"SCRIBE(\n" << page->str() << "\n)SCRIBE\")\n";
        }
        sourceStringStream << ";\n" << std::endl << std::endl;

        sourceStringStream << "gpu::ShaderPointer " << targetName << "::getShader() {" << std::endl;
        sourceStringStream << "\tif (_shader==nullptr) {" << std::endl;
        sourceStringStream << "\t\t_shader = gpu::Shader::create" << shaderCreateString[type] << "(std::string(_source));" << std::endl;
        sourceStringStream << "\t}" << std::endl;
        sourceStringStream << "\treturn _shader;" << std::endl;
        sourceStringStream << "}\n" << std::endl;

        // Destination stream
        if (!destFilename.empty()) {
            std::fstream sourceFile;
            sourceFile.open(sourceFileName, std::fstream::out);
            if (!sourceFile.is_open()) {
                cerr << "Scribe output file <" << sourceFileName << "> failed to open." << endl;
                return 1;
            }
            sourceFile << sourceStringStream.str();
        } else {
            cerr << sourceStringStream.str();
        }
    } else {
        // Destination stream
        if (!destFilename.empty()) {
            std::fstream destFileStream;
            destFileStream.open(destFilename, std::fstream::out);
            if (!destFileStream.is_open()) {
                cerr << "Scribe output file <" << destFilename << "> failed to open." << endl;
                return 1;
            }

            destFileStream << destStringStream.str();
        } else {
            cerr << destStringStream.str();
        }
    }

    return 0;

}
