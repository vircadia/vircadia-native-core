//
//  TextTemplate.cpp
//  tools/shaderScribe/src
//
//  Created by Sam Gateau on 12/15/2014.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#include "TextTemplate.h"

#include <stdarg.h>
#include <fstream>
#include <sstream>
#include <algorithm>

typedef TextTemplate::Block::Pointer BlockPointer;
typedef TextTemplate::Config::Pointer ConfigPointer;
typedef TextTemplate::Pointer TextTemplatePointer;

const std::string TextTemplate::Tag::NULL_VAR = "_SCRIBE_NULL";

//-----------------------------------------------------------------------------
TextTemplate::Config::Config() :
    _includes(),
    _funcs(),
    _logStream(&std::cout),
    _numErrors(0),
    _includerCallback(TextTemplate::loadFile) {
    _paths.push_back("");
}

const TextTemplatePointer TextTemplate::Config::addInclude(const ConfigPointer& config, const char* include) {
    if (!config) {
        return TextTemplatePointer();
    }

    TextTemplatePointer included = config->findInclude(include);
    if (included) {
        return included;
    }

    // INcluded doest exist yet so let's try to create it
    String includeStream;
    if (config->_includerCallback(config, include, includeStream)) {
        // ok, then create a new Template on the include file with this as lib
        included = std::make_shared<TextTemplate>(include, config);

        std::stringstream src(includeStream);

        int nbErrors = included->parse(src);
        if (nbErrors > 0) {
            included->logError(included->_root, "File failed to parse, not included");
            return TextTemplatePointer();
        }
        config->_includes.insert(Includes::value_type(include, included));
        return included;
    }

    return TextTemplatePointer();
}

const TextTemplatePointer TextTemplate::Config::findInclude(const char* include) {
    Includes::iterator includeIt = _includes.find(String(include));
    if (includeIt != _includes.end()) {
        return (*includeIt).second;
    } else {
        return TextTemplatePointer();
    }
}

void TextTemplate::Config::addIncludePath(const char* path) {
    _paths.push_back(String(path));
}

bool TextTemplate::loadFile(const ConfigPointer& config, const char* filename, String& source) {
    String sourceFile(filename);
    String fullfilename;
    for (unsigned int i = 0; i < config->_paths.size(); i++) {
        fullfilename = config->_paths[i] + sourceFile;
        std::ifstream ifs;
        ifs.open(fullfilename.c_str());
        if (ifs.is_open()) {
            std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            source = str;
            ifs.close();
            return (source.length() > 0);
        }
    }

    return false;
}

TextTemplate::Funcs::Funcs() :
    _funcs() {
}

TextTemplate::Funcs::~Funcs() {
}


const BlockPointer TextTemplate::Funcs::findFunc(const char* func) {
    map::iterator it = _funcs.find(String(func));
    if (it != _funcs.end()) {
        return (*it).second;
    } else {
        return BlockPointer();
    }
}

const BlockPointer TextTemplate::Funcs::addFunc(const char* func, const BlockPointer& funcBlock) {
    BlockPointer included = findFunc(func);
    if (! included) {
        _funcs.insert(map::value_type(func, funcBlock));
    }
    return included;
}

void TextTemplate::Block::addNewBlock(const Pointer& parent, const Pointer& block) {
    if (parent) {
        parent->blocks.push_back(block);
        block->parent = parent;
    }
}

const BlockPointer& TextTemplate::Block::getCurrentBlock(const Pointer& block) {
    if (block && block->command.isBlockEnd()) {
        return block->parent;
    } else {
        return block;
    }
}

void TextTemplate::logError(const Block::Pointer& block, const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);

    char buff[256];
    sprintf(buff, fmt, argp);

    _numErrors++;
    log() << block->sourceName << " Error >>" << buff << std::endl;

    int level = 1;
    displayTree(std::cerr, level);

}

bool TextTemplate::grabUntilBeginTag(std::istream* str, std::string& grabbed, Tag::Type& tagType) {
    std::stringstream dst;
    while (!str->eof()) {
        
        std::string datatoken;
        getline((*str), datatoken, Tag::BEGIN);
        dst << datatoken;
            
        char next = str->peek();
        if (next == Tag::VAR) {
            tagType = Tag::VARIABLE;
            grabbed = dst.str();
            str->get(); // skip tag char
            return true;
        } else if (next == Tag::COM) {
            tagType = Tag::COMMAND;
            grabbed = dst.str();
            str->get(); // skip tag char
            return true;
        } else if (next == Tag::REM) {
            tagType = Tag::REMARK;
            grabbed = dst.str();
            str->get(); // skip tag char
            return true;
        } else {
            if (!str->eof()) {
                // false positive, just found the Tag::BEGIN without consequence
                dst << Tag::BEGIN;
                // keep searching
            } else {
                // end of the file finishing with no tag
                tagType = Tag::INVALID;
                grabbed = dst.str();
                return true;
            }
        }
    }

    return false;
}

bool TextTemplate::grabUntilEndTag(std::istream* str, std::string& grabbed, Tag::Type& tagType) {
    std::stringstream dst;

    // preEnd char depends on tag type
    char preEnd = Tag::COM;
    if (tagType == Tag::VARIABLE) {
        preEnd = Tag::VAR;
    } else if (tagType == Tag::REMARK) {
        preEnd = Tag::REM;
    }

    while (!str->eof() && !str->fail()) {
        // looking for the end of the tag means find the next preEnd
        std::string dataToken;
        getline((*str), dataToken, preEnd);
        char end = str->peek();

        dst << dataToken;

        // and if the next char is Tag::END then that's it
        if (end == Tag::END) {
            grabbed = dst.str();
            str->get(); // eat the Tag::END
            return true;
        } else {
            // false positive, keep on searching
            dst << preEnd;
        }
    }
    grabbed = dst.str();
    return false;
}

bool TextTemplate::stepForward(std::istream* str, std::string& grabbed, std::string& tag, Tag::Type& tagType,
    Tag::Type& nextTagType) {
    if (str->eof()) {
        return false;
    }
            
    if (!_steppingStarted) {
        _steppingStarted = true;
        return grabUntilBeginTag(str, grabbed, nextTagType);
    }

    // Read from the last opening Tag::BEGIN captured to the next Tag::END
    if (grabUntilEndTag(str, tag, tagType)) {
        // skip trailing space and new lines only after Command or Remark tag block
        if ((tagType == Tag::COMMAND) || (tagType == Tag::REMARK)) {
            while (!str->eof() && !str->fail()) {
                char c = str->peek();
                if ((c == ' ') || (c == '\t') || (c == '\n')) {
                    str->get();
                } else {
                    break;
                }
            }
        }

        grabUntilBeginTag(str, grabbed, nextTagType);
    }

    return true; //hasElement;
}

const BlockPointer TextTemplate::processStep(const BlockPointer& block, std::string& grabbed, std::string& tag,
    Tag::Type& tagType) {
    switch (tagType) {
        case Tag::INVALID:
            block->ostr << grabbed;
            return block;
            break;
        case Tag::VARIABLE:
            return processStepVar(block, grabbed, tag);
            break;
        case Tag::COMMAND:
            return processStepCommand(block, grabbed, tag);
            break;
        case Tag::REMARK:
            return processStepRemark(block, grabbed, tag);
            break;
    }

    logError(block, "Invalid tag");
    return block;
}

bool TextTemplate::grabFirstToken(String& src, String& token, String& reminder) {
    bool goOn = true;
    std::string::size_type i = 0;
    while (goOn && (i < src.length())) {
        char c = src[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_') || (c == '.') || (c == '[') || (c == ']')) {
            token += c;
        } else {
            if (!token.empty()) {
                reminder = src.substr(i);
                return true;
            }
        }
        i++;
    }

    return (!token.empty());
}

bool TextTemplate::convertExpressionToArguments(String& src, std::vector< String >& arguments) {
    std::stringstream str(src);
    String token;

    while (!str.eof()) {
        str >> token;
        if (!str.fail()) {
            arguments.push_back(token);
        }
    }

    return true;
}

bool TextTemplate::convertExpressionToDefArguments(String& src, std::vector< String >& arguments) {
    if (src.empty()) {
        return false;
    }

    std::stringstream argstr(src);
    std::stringstream dest;
    bool inVar = false;
    while (!argstr.eof()) {
        // parse the value of a var, try to find a VAR, so look for the pattern BEGIN,VAR ... VAR,END
        String token;
        char tag;
        if (!inVar) {
            getline(argstr, token, Tag::BEGIN);
            dest << token;
            tag = argstr.peek();
        } else {
            getline(argstr, token, Tag::END);
            dest << token;
            tag = token.back();
        }

        if (tag == Tag::VAR) {
            if (!inVar) {
                // real var coming:
                arguments.push_back(dest.str());
                inVar = true;
            } else {
                // real var over
                arguments.push_back(dest.str());
                inVar = false;
            }
        } else {
            if (argstr.eof()) {
                arguments.push_back(dest.str());
            } else {
                // put back the tag char stolen
                dest << (!inVar ? Tag::BEGIN : Tag::END);
            }
        }
    }

    return true;
}

bool TextTemplate::convertExpressionToFuncArguments(String& src, std::vector< String >& arguments) {
    if (src.empty()) {
        return false;
    }
    std::stringstream streamSrc(src);
    String params;
    getline(streamSrc, params, '(');
    getline(streamSrc, params, ')');
    if (params.empty()) {
        return false;
    }

    std::stringstream str(params);
    String token;
    int nbTokens = 0;
    while (!str.eof()) {
        char c = str.peek();
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_') || (c == '.') || (c == Tag::VAR)  || (c == '[') || (c == ']')) {
            token += c;
        } else if (c == ',') {
            if (!token.empty()) {
                if (token == Tag::NULL_VAR) {
                    arguments.push_back(Tag::NULL_VAR);
                } else {
                    arguments.push_back(token);
                }
                nbTokens++;
            }
            token.clear();
        }

        str.get();
    }

    if (token.size()) {
        arguments.push_back(token);
        nbTokens++;
    }

    return (nbTokens > 0);
}

const BlockPointer TextTemplate::processStepVar(const BlockPointer& block, String& grabbed, String& tag) {
    // then look at the define
    String var = tag;
    String varName;
    String val;

    if (grabFirstToken(var, varName, val)) {
        if (!varName.empty()) {
            BlockPointer parent = Block::getCurrentBlock(block);

            // Add a new BLock 
            BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
            (newBlock->ostr) << grabbed;

            newBlock->command.type = Command::VAR;

            newBlock->command.arguments.push_back(varName);

            if (!val.empty()) {
                convertExpressionToFuncArguments(val, newBlock->command.arguments);
            }

            Block::addNewBlock(parent, newBlock);

            // dive in the new block
            return newBlock;
        }
    }

    return block;
}

const BlockPointer TextTemplate::processStepCommand(const BlockPointer& block, String& grabbed, String& tag) {
    // Grab the command name
    String command = tag;
    String commandName;

    if (grabFirstToken(command, commandName, command)) {
        if (commandName.compare("def") == 0) {
            return processStepDef(block, grabbed, command);
        } else if (commandName.compare("if") == 0) {
            return processStepCommandIf(block, grabbed, command);
        } else if (commandName.compare("endif") == 0) {
            return processStepCommandEndIf(block, grabbed, command);
        } else if (commandName.compare("else") == 0) {
            return processStepCommandElse(block, grabbed, command);
        } else if (commandName.compare("elif") == 0) {
            return processStepCommandElif(block, grabbed, command);
        } else if (commandName.compare("include") == 0) {
            return processStepInclude(block, grabbed, command);
        } else if (commandName.compare("func") == 0) {
            return processStepFunc(block, grabbed, command);
        } else if (commandName.compare("endfunc") == 0) {
            return processStepEndFunc(block, grabbed, command);
        }
    }

    return block;
}

const BlockPointer TextTemplate::processStepDef(const BlockPointer& block, String& grabbed, String& def) {
    // then look at the define
    String varName;
    String val;

    if (!grabFirstToken(def, varName, val)) {
        logError(block, "Invalid Var name in a <def> tag");
        return block;
    }

    BlockPointer parent = Block::getCurrentBlock(block);

    // Add a new BLock 
    BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
    (newBlock->ostr) << grabbed;

    newBlock->command.type = Command::DEF;
    newBlock->command.arguments.push_back(varName);
    if (!val.empty()) {
        // loose first character which should be a white space
        val = val.substr(val.find_first_not_of(' '));
        convertExpressionToDefArguments(val, newBlock->command.arguments);
    }

    Block::addNewBlock(parent, newBlock);

    // dive in the new block
    return newBlock;
}


const BlockPointer TextTemplate::processStepCommandIf(const BlockPointer& block, String& grabbed, String& expression) {
    BlockPointer parent = Block::getCurrentBlock(block);

    // Add a new BLock depth
    BlockPointer newIfBlock = std::make_shared<Block>(_root->sourceName);
    newIfBlock->command.type = Command::IFBLOCK;

    Block::addNewBlock(parent, newIfBlock);

    BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
    (newBlock->ostr) << grabbed;

    newBlock->command.type = Command::IF;
    convertExpressionToArguments(expression, newBlock->command.arguments);

    Block::addNewBlock(newIfBlock, newBlock);

    // dive in the new If branch
    return newBlock;
}

const BlockPointer TextTemplate::processStepCommandEndIf(const BlockPointer& block, String& grabbed, String& expression) {
    BlockPointer parent = Block::getCurrentBlock(block);

    // are we in a if block ?
    if ((parent->command.type == Command::IF) 
        ||  (parent->command.type == Command::ELIF)
        ||  (parent->command.type == Command::ELSE)) {
        BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
        (newBlock->ostr) << grabbed;

        newBlock->command.type = Command::ENDIF;
        newBlock->command.arguments.push_back(expression);

        Block::addNewBlock(parent->parent->parent, newBlock);

        return newBlock;
    } else {
        logError(block, "Invalid <endif> block, not in a <if> block");
        return block;
    }

    return block;
}

const BlockPointer TextTemplate::processStepCommandElse(const BlockPointer& block, String& grabbed, String& expression) {
    BlockPointer parent = Block::getCurrentBlock(block);

    // are we in a if block ?
    if ((parent->command.type == Command::IF) 
        ||  (parent->command.type == Command::ELIF)) {
        // All good go back to the IfBlock
        parent = parent->parent;

        // Add a new BLock depth
        BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
        newBlock->ostr << grabbed;
        newBlock->command.type = Command::ELSE;
        newBlock->command.arguments.push_back(expression);

        Block::addNewBlock(parent, newBlock);

        // dive in the new block
        return newBlock;

    } else if ((block)->command.type == Command::ELSE) {
        logError(block, "Invalid <elif> block, in a <if> block but after a <else> block");
        return block;
    } else {
        logError(block, "Invalid <elif> block, not in a <if> block");
        return block;
    }
}

const BlockPointer TextTemplate::processStepCommandElif(const BlockPointer& block, String& grabbed, String& expression) {
    BlockPointer parent = Block::getCurrentBlock(block);

    // are we in a if block ?
    if ((parent->command.type == Command::IF)
        ||  (parent->command.type == Command::ELIF)) {
        // All good go back to the IfBlock
        parent = parent->parent;

        // Add a new BLock depth
        BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
        (newBlock->ostr) << grabbed;

        newBlock->command.type = Command::ELIF;
        convertExpressionToArguments(expression, newBlock->command.arguments);

        Block::addNewBlock(parent, newBlock);

        // dive in the new block
        return newBlock;

    } else if (parent->command.type == Command::ELSE) {
        logError(block, "Invalid <elif> block, in a <if> block but after a <else> block");
        return block;
    } else {
        logError(block, "Invalid <elif> block, not in a <if> block");
        return block;
    }
}

const BlockPointer TextTemplate::processStepRemark(const BlockPointer& block, String& grabbed, String& tag) {
    // nothing to do :)

    // no need to think, let's just add the grabbed text
    (block->ostr) << grabbed;

    return block;
}

const BlockPointer TextTemplate::processStepInclude(const BlockPointer& block, String& grabbed, String& include) {
    BlockPointer parent = Block::getCurrentBlock(block);

    // Add a new BLock 
    BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
    (newBlock->ostr) << grabbed;

    newBlock->command.type = Command::INCLUDE;
    convertExpressionToArguments(include, newBlock->command.arguments);

    Block::addNewBlock(parent, newBlock);

    TextTemplatePointer inc = Config::addInclude(_config, newBlock->command.arguments.front().c_str());
    if (!inc) {
        logError(newBlock, "Failed to include %s", newBlock->command.arguments.front().c_str());
    }

    // dive in the new block
    return newBlock;
}

const BlockPointer TextTemplate::processStepFunc(const BlockPointer& block, String& grabbed, String& func) {
    // then look at the define
    String varName;
    String var;

    if (!grabFirstToken(func, varName, var)) {
        logError(block, "Invalid func name <%s> in a <func> block", func.c_str());
        return block;
    }

    // DOes the func already exists ?
    if (_config->_funcs.findFunc(varName.c_str())) {
        logError(block, "Declaring a new func named <%s> already exists", func.c_str());
        return block;
    }

    BlockPointer parent = Block::getCurrentBlock(block);

    // Add a new BLock 
    BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
    (newBlock->ostr) << grabbed;

    newBlock->command.type = Command::FUNC;
    newBlock->command.arguments.push_back(varName);
    convertExpressionToFuncArguments(var, newBlock->command.arguments);

    _config->_funcs.addFunc(varName.c_str(), newBlock);

    Block::addNewBlock(parent, newBlock);

    // dive in the new block
    return newBlock;
}

const BlockPointer TextTemplate::processStepEndFunc(const BlockPointer& block, String& grabbed, String& tag) {
    BlockPointer parent = Block::getCurrentBlock(block);

    // are we in a func block ?
    if (parent->command.type == Command::FUNC) {
        // Make sure the FUnc has been declared properly
        BlockPointer funcBlock = _config->_funcs.findFunc(parent->command.arguments.front().c_str());
        if (funcBlock != parent) {
            logError(block, "Mismatching <func> blocks");
            return BlockPointer();
        }

        // Everything is cool , so let's unplugg the FUnc block from this tree and just put the EndFunc block

        BlockPointer newBlock = std::make_shared<Block>(_root->sourceName);
        (newBlock->ostr) << grabbed;

        newBlock->command.type = Command::ENDFUNC;
        convertExpressionToArguments(tag, newBlock->command.arguments);

        newBlock->parent = parent->parent;

        parent->parent->blocks.back() = newBlock;

        parent->parent = 0;

        // dive in the new block
        return newBlock;
    } else {
        logError(block, "Invalid <endfunc> block, not in a <func> block");
        return BlockPointer();
    }

    return block;
}

TextTemplate::TextTemplate(const String& name, const ConfigPointer& externalConfig) :
    _config(externalConfig),
    _root(new Block(name)),
    _numErrors(0),
    _steppingStarted(false) {
}

TextTemplate::~TextTemplate() {
}

int TextTemplate::scribe(std::ostream& dst, std::istream& src, Vars& vars) {
    int nbErrors = parse(src);

    if (nbErrors == 0) {
        return generate(dst, vars);
    }
    return nbErrors;
}

int TextTemplate::generateTree(std::ostream& dst, const BlockPointer& block, Vars& vars) {
    BlockPointer newCurrentBlock;
    int numPasses = evalBlockGeneration(dst, block, vars, newCurrentBlock);
    for (int passNum= 0; passNum < numPasses; passNum++) {
        dst << newCurrentBlock->ostr.str();

        for (auto child : newCurrentBlock->blocks) {
            generateTree(dst, child, vars);
        }
    }

    return _numErrors;
}

int TextTemplate::evalBlockGeneration(std::ostream& dst, const BlockPointer& block, Vars& vars, BlockPointer& branch) {
    switch (block->command.type) {
        case Command::BLOCK: {
            branch = block;
            return 1;
        }
        break;
        case Command::VAR: {
            Vars::iterator it = vars.find(block->command.arguments.front());
            if (it != vars.end()) {
                dst << (*it).second;
            } else {
                BlockPointer funcBlock = _config->_funcs.findFunc(block->command.arguments.front().c_str());
                if (funcBlock) {
                    // before diving in the func tree, let's modify the vars with the local defs:
                    int nbParams = (int)std::min(block->command.arguments.size(),
                                                 funcBlock->command.arguments.size());
                    std::vector< String > paramCache;
                    paramCache.push_back("");
                    String val;
                    for (int i = 1; i < nbParams; i++) {
                        val = block->command.arguments[i];
                        if ((val[0] == Tag::VAR) && (val[val.length()-1] == Tag::VAR)) {
                            val = val.substr(1, val.length()-2);
                            Vars::iterator it = vars.find(val);
                            if (it != vars.end()) {
                                val = (*it).second;
                            } else {
                                val = Tag::NULL_VAR;
                            }
                        }

                        Vars::iterator it = vars.find(funcBlock->command.arguments[i]);
                        if (it != vars.end()) {
                            paramCache.push_back((*it).second);
                            (*it).second = val;
                        } else {
                            if (val != Tag::NULL_VAR) {
                                vars.insert(Vars::value_type(funcBlock->command.arguments[i], val));
                            }

                            paramCache.push_back(Tag::NULL_VAR);
                        }
                    }

                    generateTree(dst, funcBlock, vars);

                    for (int i = 1; i < nbParams; i++) {
                        if (paramCache[i] == Tag::NULL_VAR) {
                            vars.erase(funcBlock->command.arguments[i]);
                        } else {
                            vars[funcBlock->command.arguments[i]] = paramCache[i];
                        }
                    }
                }
            }
            branch = block;
            return 1;
        }
        break;
        case Command::IFBLOCK: {
            // ok, go through the branches and pick the first one that goes
            for (auto child: block->blocks) {
                int numPasses = evalBlockGeneration(dst, child, vars, branch);
                if (numPasses > 0) {
                    return numPasses;
                }
            }
        }
        break;
        case Command::IF:
        case Command::ELIF: {
            if (!block->command.arguments.empty()) {
                // Just one argument means check for the var beeing defined
                if (block->command.arguments.size() == 1) {
                    Vars::iterator it = vars.find(block->command.arguments.front());
                    if (it != vars.end()) {
                        branch = block;
                        return 1;
                    }
                } else if (block->command.arguments.size() == 2) {
                    if (block->command.arguments[0].compare("not") == 0) {
                        Vars::iterator it = vars.find(block->command.arguments[1]);
                        if (it == vars.end()) {
                            branch = block;
                            return 1;
                        }
                    }
                } else if (block->command.arguments.size() == 3) {
                    if (block->command.arguments[1].compare("and") == 0) {
                        Vars::iterator itL = vars.find(block->command.arguments[0]);
                        Vars::iterator itR = vars.find(block->command.arguments[2]);
                        if ((itL != vars.end()) && (itR != vars.end())) {
                            branch = block;
                            return 1;
                        }
                    } else if (block->command.arguments[1].compare("or") == 0) {
                        Vars::iterator itL = vars.find(block->command.arguments[0]);
                        Vars::iterator itR = vars.find(block->command.arguments[2]);
                        if ((itL != vars.end()) || (itR != vars.end())) {
                            branch = block;
                            return 1;
                        }
                    } else if (block->command.arguments[1].compare("==") == 0) {
                        Vars::iterator itL = vars.find(block->command.arguments[0]);
                        if (itL != vars.end()) {
                            if ((*itL).second.compare(block->command.arguments[2]) == 0) {
                                branch = block;
                                return 1;
                            }
                        }
                    }
                }

            }
            return 0;
        }
        break;
        case Command::ELSE: {
            branch = block;
            return 1;
        }
        break;
        case Command::ENDIF: {
            branch = block;
            return 1;
        }
        break;
        case Command::DEF: {
            if (block->command.arguments.size()) {
                // THe actual value of the var defined sneeds to be evaluated:
                String val;
                for (unsigned int t = 1; t < block->command.arguments.size(); t++) {
                    // detect if a param is a var
                    auto len = block->command.arguments[t].length();
                    if ((block->command.arguments[t][0] == Tag::VAR)
                        && (block->command.arguments[t][len - 1] == Tag::VAR)) {
                        String var = block->command.arguments[t].substr(1, len - 2);
                        Vars::iterator it = vars.find(var);
                        if (it != vars.end()) {
                            val += (*it).second;
                        }
                    } else {
                        val += block->command.arguments[t];
                    }
                }

                Vars::iterator it = vars.find(block->command.arguments.front());
                if (it == vars.end()) {
                    vars.insert(Vars::value_type(block->command.arguments.front(), val));
                } else {
                    (*it).second = val;
                }

                branch = block;
                return 1;
            } else {
                branch = block;
                return 0;
            }
        }
        break;

        case Command::INCLUDE: {
            TextTemplatePointer include = _config->findInclude(block->command.arguments.front().c_str());
            if (include && !include->_root->blocks.empty()) {
                if (include->_root) {
                    generateTree(dst, include->_root, vars);
                }
            }

            branch = block;
            return 1;
        }
        break;

        case Command::FUNC: {
            branch = block;
            return 1;
        }
        break;

        case Command::ENDFUNC: {
            branch = block;
            return 1;
        }
        break;

        default: {
        }
    }

    return 0;
}


int TextTemplate::parse(std::istream& src) {
    _root->command.type = Command::BLOCK;
    BlockPointer currentBlock = _root;
    _numErrors = 0;

    // First Parse the input file
    int nbLoops = 0;
    bool goOn = true;
    Tag::Type tagType = Tag::INVALID;
    Tag::Type nextTagType = Tag::INVALID;
    while (goOn) {
        std::string data;
        std::string tag;
        if (stepForward(&src, data, tag, tagType, nextTagType)) {
            currentBlock = processStep(currentBlock, data, tag, tagType);
        } else {
            goOn = false;
        }

        tagType = nextTagType;
        nbLoops++;
    }

    return _numErrors;
}

int TextTemplate::generate(std::ostream& dst, Vars& vars) {
    return generateTree(dst, _root, vars);
}

void TextTemplate::displayTree(std::ostream& dst, int& level) const {
    Block::displayTree(_root, dst, level);
}

void TextTemplate::Block::displayTree(const BlockPointer& block, std::ostream& dst, int& level) {
    String tab(level * 2, ' ');

    const String BLOCK_TYPE_NAMES[] = {
        "VAR",
        "BLOCK",
        "FUNC",
        "ENDFUNC",
        "IFBLOCK",
        "IF",
        "ELIF",
        "ELSE",
        "ENDIF",
        "FOR",
        "ENDFOR",
        "INCLUDE",
        "DEF"
    };

    dst << tab << "{ " << BLOCK_TYPE_NAMES[block->command.type] << ":";
    if (!block->command.arguments.empty()) {
        for (auto arg: block->command.arguments) {
            dst << " " << arg;
        }
    }
    dst << std::endl;

    level++;
    for (auto sub: block->blocks) {
        displayTree(sub, dst, level);
    }
    level--;

    dst << tab << "}" << std::endl;
}

void TextTemplate::Config::displayTree(std::ostream& dst, int& level) const {
    String tab(level * 2, ' ');

    level++;
    dst << tab << "Includes:" << std::endl;
    for (auto inc: _includes) {
        dst << tab << tab << inc.first << std::endl;
        inc.second->displayTree(dst, level);
    }
    dst << tab << "Funcs:" << std::endl;
    for (auto func: _funcs._funcs) {
        dst << tab << tab << func.first << std::endl;
        TextTemplate::Block::displayTree( func.second, dst, level);
    }
    level--;
}
