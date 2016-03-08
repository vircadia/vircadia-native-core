
//
//  TextTemplate.cpp
//  tools/shaderScribe/src
//
//  Created by Sam Gateau on 12/15/2014.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#ifndef hifi_TEXT_TEMPLATE_H
#define hifi_TEXT_TEMPLATE_H

#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>

class TextTemplate {
public:
    typedef std::shared_ptr< TextTemplate > Pointer;
    typedef std::string String;
    typedef std::vector< String > StringVector;
    typedef std::map< String, String > Vars;
    typedef std::map< String, TextTemplate::Pointer > Includes;

    class Tag {
    public:
        enum Type {
            VARIABLE = 0,
            COMMAND,
            REMARK,
            INVALID = -1,
        };

        static const char BEGIN = '<';
        static const char END = '>';

        static const char VAR = '$';
        static const char COM = '@';
        static const char REM = '!';
        
        static const std::string NULL_VAR;
    };

    class Command {
    public:
        typedef std::vector< Command > vector;

        enum Type {
            VAR = 0,
            BLOCK,
            FUNC,
            ENDFUNC,
            IFBLOCK,
            IF,
            ELIF,
            ELSE,
            ENDIF,
            FOR,
            ENDFOR,
            INCLUDE,
            DEF,
        };

        Type type;
        std::vector< String > arguments;

        bool isBlockEnd() {
            switch (type) {
                case ENDFUNC:
                case ENDIF:
                case ENDFOR:
                case INCLUDE:
                case DEF:
                case VAR:
                    return true;
                default:
                    return false;
            }
        }
    };

    class Block {
    public:
        typedef std::shared_ptr<Block> Pointer;
        typedef std::vector< Block::Pointer > Vector;

        Block::Pointer parent;
        Command command;
        Vector blocks;
        std::stringstream ostr;

        String sourceName;

        Block(const String& sourceFilename) :
            sourceName(sourceFilename) {}

        static void addNewBlock(const Block::Pointer& parent, const Block::Pointer& block);
        static const Block::Pointer& getCurrentBlock(const Block::Pointer& block);

        static void displayTree(const Block::Pointer& block, std::ostream& dst, int& level);
    };

    class Funcs {
    public:
        typedef std::map< String, Block::Pointer > map;

        Funcs();
        ~Funcs();

        const Block::Pointer findFunc(const char* func);
        const Block::Pointer addFunc(const char* func, const Block::Pointer& root);

        map _funcs;
    protected:
    };

    class Config {
    public:
        typedef std::shared_ptr< Config > Pointer;
        typedef bool (*IncluderCallback) (const Config::Pointer& config, const char* filename, String& source);

        Includes        _includes;
        Funcs           _funcs;
        std::ostream*   _logStream;
        int             _numErrors;
        IncluderCallback _includerCallback;
        StringVector    _paths;

        Config();

        static const TextTemplate::Pointer addInclude(const Config::Pointer& config, const char* include);
        const TextTemplate::Pointer findInclude(const char* include);

        void addIncludePath(const char* path);

        void displayTree(std::ostream& dst, int& level) const;
    };

    static bool loadFile(const Config::Pointer& config, const char* filename, String& source);

    TextTemplate(const String& name, const Config::Pointer& config = std::make_shared<Config>());
    ~TextTemplate();

    // Scibe does all the job of parsing an inout template stream and then gneerating theresulting stream using the vars
    int scribe(std::ostream& dst, std::istream& src, Vars& vars);

    int parse(std::istream& src);
    int generate(std::ostream& dst, Vars& vars);

    const Config::Pointer config() { return _config; }

    void displayTree(std::ostream& dst, int& level) const;

protected:
    Config::Pointer _config;
    Block::Pointer _root;
    int _numErrors;
    bool _steppingStarted;

    bool grabUntilBeginTag(std::istream* str, String& grabbed, Tag::Type& tagType);
    bool grabUntilEndTag(std::istream* str, String& grabbed, Tag::Type& tagType);

    bool stepForward(std::istream* str, String& grabbed, String& tag, Tag::Type& tagType, Tag::Type& nextTagType);

    bool grabFirstToken(String& src, String& token, String& reminder);
    bool convertExpressionToArguments(String& src, std::vector< String >& arguments);
    bool convertExpressionToDefArguments(String& src, std::vector< String >& arguments);
    bool convertExpressionToFuncArguments(String& src, std::vector< String >& arguments);

    // Filter between var, command or comments
    const Block::Pointer processStep(const Block::Pointer& block, String& grabbed, String& tag, Tag::Type& tagType);
    const Block::Pointer processStepVar(const Block::Pointer& block, String& grabbed, String& tag);
    const Block::Pointer processStepCommand(const Block::Pointer& block, String& grabbed, String& tag);
    const Block::Pointer processStepRemark(const Block::Pointer& block, String& grabbed, String& tag);

    // Define command
    const Block::Pointer processStepDef(const Block::Pointer& block, String& grabbed, String& tag);

    // If commands
    const Block::Pointer processStepCommandIf(const Block::Pointer& block, String& grabbed, String& expression);
    const Block::Pointer processStepCommandEndIf(const Block::Pointer& block, String& grabbed, String& expression);
    const Block::Pointer processStepCommandElif(const Block::Pointer& block, String& grabbed, String& expression);
    const Block::Pointer processStepCommandElse(const Block::Pointer& block, String& grabbed, String& expression);

    // Include command
    const Block::Pointer processStepInclude(const Block::Pointer& block, String& grabbed, String& tag);

    // Function command
    const Block::Pointer processStepFunc(const Block::Pointer& block, String& grabbed, String& tag);
    const Block::Pointer processStepEndFunc(const Block::Pointer& block, String& grabbed, String& tag);

    // Generation
    int generateTree(std::ostream& dst, const Block::Pointer& block, Vars& vars);
    int evalBlockGeneration(std::ostream& dst, const Block::Pointer& block, Vars& vars, Block::Pointer& branch);

    // Errors
    std::ostream& log() { return (* _config->_logStream); }
    void logError(const Block::Pointer& block, const char* error, ...);
};

#endif
