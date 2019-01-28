// Dependencies
const htmlclean = require('htmlclean');
const fs = require('fs');
const path = require('path');
const pretty = require('pretty');
const cheerio = require('cheerio');
const rimraf = require('rimraf');
const dedent = require('dedent-js');

// Arg Vars
const copyLocal = process.argv[2];
console.log("copyLocal:", copyLocal);
let targetTemplateDirectory = '';
let targetMDDirectory = '';
if (copyLocal) {
    targetTemplateDirectory = process.argv[3];
    targetMDDirectory = process.argv[4];;
}

// Required directories
let dir_out = path.join(__dirname, 'out');

let dir_grav = path.join(dir_out, 'grav');
let dir_css = path.join(dir_grav, 'css');
let dir_js = path.join(dir_grav, 'js');
let dir_template = path.join(dir_grav, 'templates');

let dir_md = path.join(dir_grav, '08.api-reference');
let dir_md_objects = path.join(dir_md, '02.Objects');
let dir_md_namespaces = path.join(dir_md, '01.Namespaces');
let dir_md_globals = path.join(dir_md, '03.Globals');

// Array to itterate over and create if doesn't exist
let dirArray = [dir_grav, dir_css, dir_js, dir_template, dir_md, dir_md_objects, dir_md_namespaces, dir_md_globals];

// Base Grouping Directories for MD files
let baseMDDirectories = ["API-Reference", "Globals", "Namespaces", "Objects"];

// Maps for directory names
let map_dir_md = {
    "API-Reference": dir_md,
    "Globals": dir_md_globals,
    "Objects": dir_md_objects,
    "Namespaces": dir_md_namespaces,
    "Class": dir_md_objects,
    "Namespace": dir_md_namespaces,
    "Global": dir_md_globals
}

// Map for Links
let map_links = {
    "Global": "globals",
    "Namespace": "namespaces",
    "Class": "objects"
}

// Mapping for GroupNames and Members
let groupNameMemberMap = {
    "Objects": [],
    "Namespaces": [],
    "Globals": []
}

// Html variables to be handle regex replacements
const html_reg_static = /<span class="type-signature">\(static\)<\/span>/g
const html_reg_title = /\<h1.+?\>.+?\<\/h1\>/g;
const html_reg_htmlExt = /\.html/g;
const html_reg_objectHeader = /<header>[\s\S]+?<\/header>/;
const html_reg_objectSpanNew = /<h5 class="name"[\s\S]+?<\/span><\/h5>/;
const html_reg_propertiesHeaderEdit = '<h4 class="subsection-title">Properties:</h4>';
const html_reg_propertiesHeaderEdit_Replace = '<h4 class="subsection-title">Properties</h4>';
const html_reg_typeEdit = /(<h5>Returns[\s\S]*?Type)(<\/dt[\s\S]*?type">)(.*?)(<\/span><\/dd>[\s\S]*?<\/dl>)/g;
const html_reg_typeEdit_replace = '$1: $3</dt></dl>'
const html_reg_methodSize = /(<h4)( class="name"[\s\S]*?)(<\/h4>)/g;
const html_reg_methodSize_replace = '<h5$2</h5>';
const html_reg_findByName = '<h5 class="name"';
const html_reg_findByTitle = '<h1>';
const html_reg_findByMethod = '<h3 class="subsection-title">Methods</h3>'
const html_reg_findByMethod_replace = '<h4 class="subsection-title">Methods</h4>'
const html_reg_findByArticleClose = `</article>`
const html_reg_signalTitle = `<h4 class="subsection-title">Signals</h4>`;
const html_reg_typeDefinitonsTitle = /<h3 class="subsection-title">Type Definitions<\/h3>/;
const html_reg_typeDefinitonsTitle_replace = `<h4 class="subsection-title">Type Definitions</h4>`
const html_reg_classDefinitonsTitle = /<h3 class="subsection-title">Classes<\/h3>/;
const html_reg_classDefinitonsTitle_replace = `<h4 class="subsection-title">Classes</h4>`
const html_reg_firstDivClose = `</div>`;
const html_reg_allNonHTTPLinks = /(<a href=")(?!http)([\s\S]+?)(">)/g;
const html_reg_allHTTPLinks = /(<a href=")(http)([\s\S]+?)(">)/g;
const html_reg_pretty = /(<pre class="prettyprint">)([\s\S]*?)(<\/pre>)/g;
const html_reg_pretty_replace = "<pre>$2<\/pre>";
const html_reg_availableIn = /(<table>[\s\S]+?Available in:[\s\S]+?<\/table>)/g;
const html_reg_findControllerCuratedList = /<h5>Functions<\/h5>[\s\S]*?<p>Input Recordings[\s\S]*?<\/ul>/g
const html_reg_findEntityMethods = /<h5>Entity Methods:[\s\S]+?<\/ul>/g;
const html_reg_EntityMethodsHeader = '<h5>Entity Methods:</h5>';
const html_reg_EntityMethodsHeader_replace = '<h5>Entity Methods</h5>';
const html_reg_dlClassDetails = /<dl class="details"><\/dl>/g
const html_reg_typeDefType = /(<h5>)(Type:)(<\/h5>[\s\S]*?)(<span[\s\S]*?)(<\/li>[\s\S]*?<\/ul>)/g;
const html_reg_typeDefType_replace = `<dl><dt>$2 $4</dl></dt>`;
const html_reg_returnSize = /<h5>Returns:<\/h5>/g;
const html_reg_returnSize_replace = '<h6>Returns:<\/h6>';
const html_reg_depreciated = /(<dt class="important tag-deprecated"[\s\S]+?<\/dt>[\s\S]+?)(<dd>)([\s\S]+?<ul[\s\S]+?<li>)([\s\S]+?)(<\/ul>[\s\S]+?)(<\/dd>)/g;
const html_reg_depreciated_replace = '<em><strong>$1</strong><div>$4</div></em>'

// Procedural functions

//remove .html from non http links
function removeHTML(match, p1, p2, p3) {
    p2 = p2.replace(".html", "");
    return [p1, p2, p3].join("");
}

// Turn links to lower case that aren't part of IDs
function allLinksToLowerCase(match, p1, p2, p3) {
    // split on id # and make sure only the preceding is lower case
    if (p2.indexOf("#") > -1) {
        p2 = p2.split("#");
        p2 = [p2[0].toLowerCase(), "#", p2[1]].join("");
    } else {
        p2 = p2.toLowerCase();
    }
    return [p1, p2, p3].join("");
}

// Helper for fixing formatting of page links
function fixLinkGrouping(match, p1, p2, p3) {
    // Handle if referencing ID
    let count = (p2.match(/\./g) || []).length;
    if (p2.indexOf("#") > -1) {
        let split = p2.split("#");
        if (count >= 2) {
            // console.log("MULTI DOTS!");
            split = p2.split(".");
            // This is a case where we are in an object page and there are multiple levels referenced (only doing 2 levels at the moment)
            // console.log("split", split)
            return [p1, "/api-reference/", returnRightGroup(split[1].slice(0, -1)), "/", split[1], ".", split[2], p3].join("");
        }
        if (split[0] === "global") {
            return [p1, "/api-reference/", "globals", "#", split[1], p3].join("");
        }
        return [p1, "/api-reference/", returnRightGroup(split[0]), "/", p2, p3].join("");
    } else {
        // Handle if there are member references
        // console.log("count", count)
        let split;
        if (count === 1) {
            split = p2.split(".");
            return [p1, "/api-reference/", returnRightGroup(split[1]), "/", split[1], p3].join("");
        }
        return [p1, "/api-reference/", returnRightGroup(p2), "/", p2, p3].join("");
    }
}

// Return the right group for where the method or type came from
function returnRightGroup(methodToCheck) {
    for (var key in groupNameMemberMap) {
        for (i = 0; i < groupNameMemberMap[key].length; i++) {
            if (methodToCheck.toLowerCase() === groupNameMemberMap[key][i].toLowerCase()) {
                return key.toLowerCase();
            } else {
                // console.log("Couldn't find group: ", methodToCheck);
            }
        }
    }
}

// Create the actual MD file
function createMD(title, directory, needsDir, isGlobal) {
    let mdSource = makeMdSource(title);

    if (needsDir) {
        if (!fs.existsSync(directory)) {
            fs.mkdirSync(directory);
        }
    }

    let destinationMDFile = path.join(directory, `API_${title}.md`);
    fs.writeFileSync(destinationMDFile, mdSource);
}

// Create the actual Template file
function createTemplate(title, content) {
    let twigBasePartial = makeTwigFile(content);
    let destinationFile = path.join(dir_template, `API_${title}.html.twig`);
    fs.writeFileSync(destinationFile, twigBasePartial);
}

// Copy file from source to target - used for recurssive call
function copyFileSync(source, target) {
    let targetFile = target;

    // If target is a directory a new file with the same name will be created
    if (fs.existsSync(target)) {
        if (fs.lstatSync(target).isDirectory()) {
            targetFile = path.join(target, path.basename(source));
        }
    }

    fs.writeFileSync(targetFile, fs.readFileSync(source));
}

// Copy file from source to target
function copyFolderRecursiveSync(source, target) {
    var files = [];

    // Check if folder needs to be created or integrated
    var targetFolder = path.join(target, path.basename(source));
    if (!fs.existsSync(targetFolder)) {
        fs.mkdirSync(targetFolder);
    }

    // Copy
    if (fs.lstatSync(source).isDirectory()) {
        files = fs.readdirSync(source);
        files.forEach(function(file) {
            var curSource = path.join(source, file);
            if (fs.lstatSync(curSource).isDirectory()) {
                copyFolderRecursiveSync(curSource, targetFolder);
            } else {
                copyFileSync(curSource, targetFolder);
            }
        });
    }
}

// Clean up the Html
function prepareHtml(source) {
    let htmlBefore = fs.readFileSync(source, { encoding: 'utf8' });
    let htmlAfter = htmlclean(htmlBefore);
    let htmlAfterPretty = pretty(htmlAfter);
    return cheerio.load(htmlAfterPretty);
}

// Base file for MD's
function makeMdSource(title) {
    return dedent(
        `
        ---
        title: ${title}
        taxonomy:
            category:
                - docs
        visible: true
        highlight:
            enabled: false
        ---
        `
    )
}

// Base file for Templates
function makeTwigFile(contentHtml) {
    return dedent(
        `
        {% extends 'partials/base_noGit.html.twig' %}
        {% set tags = page.taxonomy.tag %}
        {% if tags %}
            {% set progress = page.collection({'items':{'@taxonomy':{'category': 'docs', 'tag': tags}},'order': {'by': 'default', 'dir': 'asc'}}) %}
        {% else %}
            {% set progress = page.collection({'items':{'@taxonomy':{'category': 'docs'}},'order': {'by': 'default', 'dir': 'asc'}}) %}
        {% endif %}
        
        {% block navigation %}
            <div id="navigation">
            {% if not progress.isFirst(page.path) %}
                <a class="nav nav-prev" href="{{ progress.nextSibling(page.path).url }}"> <img src="{{ url('theme://images/left-arrow.png') }}"></a>
            {% endif %}
        
            {% if not progress.isLast(page.path) %}
                <a class="nav nav-next" href="{{ progress.prevSibling(page.path).url }}"><img src="{{ url('theme://images/right-arrow.png') }}"></a>
            {% endif %}
            </div>
        {% endblock %}
        
        {% block content %}
            <div id="api-specific">
                <div id="body-inner">
                    <h1>{{ page.title }}</h1>
                    ${contentHtml}
                </div>
            </div>
        {% endblock %}
        `
    )
}

// Handle NameSpace Group   
function handleNamespace(title, content) {
    let destinationDirectory = path.join(map_dir_md["Namespace"], title);
    createMD(title, destinationDirectory, true);
    createTemplate(title, content);
}

// Handle Class Group       
function handleClass(title, content) {
    let destinationDirectory = path.join(map_dir_md["Class"], title);
    createMD(title, destinationDirectory, true)

    let formatedHtml = content
        .replace(html_reg_objectSpanNew, "")
    createTemplate(title, formatedHtml);
}

// Handle Global Group           
function handleGlobal(title, content) {
    createMD("Globals", map_dir_md["Global"], false, true);
    createTemplate("Globals", content);
}

// Handle Group TOCs               
function makeGroupTOC(group) {
    let mappedGroup;
    if (!Array.isArray(group)) {
        mappedGroup = groupNameMemberMap[group];
    } else {
        mappedGroup = group;
    }
    let htmlGroup = mappedGroup.map(item => {
                return dedent(
                        `
                    <div>
                        <a href="/api-reference/${
                            !Array.isArray(group)
                            ? `${group.toLowerCase()}/` + item.toLowerCase()
                            : item.toLowerCase()
                        }/">${item}</a>
                    </div>
                    `
                )
            })
            return htmlGroup.join("\n");
        }

// Handle Class TOCS
function makeClassTOC(group){
    let linkArray = []
    group.forEach( item => {
        linkArray.push(`<div><h5>${item.type}</h5></div>`)            
        item.array.forEach( link => {
        if ( link.indexOf('.') > -1 ){
                linkArray.push(`<div><a href="#${link}">${link.slice(1)}</a></div>`);
            } else {
                linkArray.push(`<div><a href="#${link}">${link}</a></div>`);
                            
            }
        })
        linkArray.push("<br>");
    })
    return linkArray.join("\n");
}

// Extract IDS for TOC
function extractIDs(groupToExtract){
    let firstLine = "";
    let id = "";
    let extractedIDs = [];
    groupToExtract.forEach((item)=>{
        firstLine = item.split("\n")[0];
        try {
            id = firstLine.split('id="')[1].split(`"`)[0];
        } catch (e){
            id = "";
        }
        if (id){
            extractedIDs.push(id)
        }
    })
    return extractedIDs;
}

// Helper for splitting up html 
// Takes: Content to split, SearchTerm to Split by, and term to End Splitting By
// Returns: [newContent after Split, Array of extracted ]
function splitBy(content, searchTerm, endSplitTerm, title){     
    let foundArray = [];
    let curIndex = -1;
    let afterCurSearchIndex = -1
    let nextIndex = 0;
    let findbyNameLength = searchTerm.length;
    let curEndSplitTermIndex = -1;
    let classHeader;
    do {
        // Find the index of where to stop searching
        curEndSplitTermIndex = content.indexOf(endSplitTerm);
        // console.log("curEndSplitTermIndex", curEndSplitTermIndex)
        // Find the index of the the next Search term
        curIndex = content.indexOf(searchTerm);
        // console.log("curIndex", curIndex)
        
        // The index of where the next search will start 
        afterCurSearchIndex = curIndex+findbyNameLength;
        // Find the content of the next Index
        nextIndex = content.indexOf(searchTerm,afterCurSearchIndex);
        // If the next index isn't found, then next index === index of the end term
        if (nextIndex === -1){
            nextIndex = curEndSplitTermIndex;
        }
        if (curIndex > curEndSplitTermIndex){
            break;
        }
        // Push from the cur index to the next found || the end term
        let contentSlice = content.slice(curIndex, nextIndex);
        if (contentSlice.indexOf(`id="${title}"`) === -1){
            foundArray.push(contentSlice);
        } else {
            classHeader = contentSlice;
        }
        
        // Remove that content
        content = content.replace(contentSlice, "");
        
        curEndSplitTermIndex = content.indexOf(endSplitTerm);
        nextIndex = content.indexOf(searchTerm,afterCurSearchIndex);
        // Handle if nextIndex goes beyond endSplitTerm
        if (nextIndex > curEndSplitTermIndex) {
            curIndex = content.indexOf(searchTerm);
            contentSlice = content.slice(curIndex, curEndSplitTermIndex);
            if (contentSlice.indexOf(`id="${title}"`) === -1){
                foundArray.push(contentSlice);
            }
            content = content.replace(contentSlice, "");
            break;
        }
    } while (curIndex > -1)
    if (classHeader){
        content = append(content, html_reg_findByArticleClose, classHeader, true);
    }
    return [content, foundArray];
}

// Split the signals and methods [Might make this more generic]
function splitMethodsSignals(allItemToSplit){
    let methodArray = [];
    let signalArray = [];
    
    allItemToSplit.forEach( (content, index) => {
        firstLine = content.split("\n")[0]; 
        if (firstLine.indexOf("{Signal}") > -1){
            signalArray.push(content);
        } else if (firstLine.indexOf("span") > -1) {
            methodArray.push(content);
        } else {
        }
    })
    return [methodArray, signalArray];
}

// Helper to append
// Takes content, the search term to appendTo, the content to append, 
// and bool if the append is before the found area
function append(content, searchTermToAppendto, contentToAppend, appendBefore){
    let contentArray = content.split("\n");
    let foundIndex = findArrayTrim(contentArray, searchTermToAppendto)
    foundIndex = appendBefore ? foundIndex : foundIndex +1
    
    contentArray.splice(foundIndex,0,contentToAppend)
    return contentArray.join("\n")
}

// Helper function for append
function findArrayTrim(array, searchTerm){
    var index = -1;
    for (var i = 0; i < array.length; i++){
        index = array[i].trim().indexOf(searchTerm.trim());
        if (index > -1){
            return i
        }
    }
    return index;
}

// Remove grav directory if exists to make sure old files aren't kept
if (fs.existsSync(dir_grav)){
    console.log("dir_grav exists");
    rimraf.sync(dir_grav);
}

// Create Grav directories in JSDOC output
dirArray.forEach(function(dir){
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir);
    }
})

// Create baseMD files
baseMDDirectories.forEach( md => {
    createMD(md, map_dir_md[md]);
})

// Read jsdoc output folder and process html files
let files = fs.readdirSync(dir_out);

// Create initial Group name member map to handle individual link
files.forEach(function (file){
    let curSource = path.join(dir_out, file);
    if (path.extname(curSource) == ".html" && path.basename(curSource, '.html') !== "index") {
        let loadedHtml = prepareHtml(curSource);
        let splitTitle = loadedHtml("title").text().split(": ");
        let groupName = splitTitle[1];
        let htmlTitle = splitTitle.pop();
        switch(groupName){
            case "Namespace":
                groupNameMemberMap["Namespaces"].push(htmlTitle);
                break;
            case "Class":
                groupNameMemberMap["Objects"].push(htmlTitle);
                break;
            default:
                console.log(`Case not handled for ${groupName}`);
        }
    }
})
files.forEach(function (file, index){
    // For testing individual files
    // if (index !== 59) return;       
    let curSource = path.join(dir_out, file);
    if (path.extname(curSource) == ".html" && path.basename(curSource, '.html') !== "index") {
        // Clean up the html source
        let loadedHtml = prepareHtml(curSource);

        // Extract the title, group name, and the main div
        let splitTitle = loadedHtml("title").text().split(": ");
        let groupName = splitTitle[1];
        let htmlTitle = splitTitle.pop();
        console.log("html title", htmlTitle)
        let mainDiv = loadedHtml("#main")

        let methodIDs = [];
        let signalIDs = [];
        let typeDefIDs = [];
        // Basic Regex HTML edits
        let currentContent = mainDiv.html()
                                .replace(html_reg_findByMethod, "") //Remove Method title to be remade later  
                                .replace(html_reg_static,"") // Remove static from the file names
                                .replace(html_reg_title,"") // Remove title 
                                .replace(html_reg_objectHeader,"") // Remove extra Object Header 
                                .replace(html_reg_dlClassDetails, "") // Remove unneccsary dlClassDetails Tag
                                .replace(html_reg_allNonHTTPLinks, removeHTML) // Remove the .html extension from all links
                                .replace(html_reg_allNonHTTPLinks, allLinksToLowerCase) // Turn all links into lowercase before ID tags
                                .replace(html_reg_allNonHTTPLinks, fixLinkGrouping) // Make sure links refer to correct grouping                                  
                                .replace(html_reg_propertiesHeaderEdit, html_reg_propertiesHeaderEdit_Replace) // Remove : from Properties
                                .replace(html_reg_typeEdit, html_reg_typeEdit_replace) // Put type on the same line
                                .replace(html_reg_returnSize, html_reg_returnSize_replace) // Make return size h6 instead of h5
                                .replace(html_reg_methodSize, html_reg_methodSize_replace) // Make method size into h5
                                .replace(html_reg_pretty, html_reg_pretty_replace) // remove the references to pretty
                                .replace(html_reg_classDefinitonsTitle, html_reg_classDefinitonsTitle_replace) // Change the class def titles
                                .replace(html_reg_depreciated, html_reg_depreciated_replace); // format depreciated better
        
        // Further HTML Manipulation
        // Make end term either Type Definitions or by the article
        let endTerm;
        let foundTypeDefinitions;
        let foundSignalsAndMethods;
        if (currentContent.indexOf("Type Definitions") > -1){
            // console.log("Found Type Definitions");
            endTerm = `<h3 class="subsection-title">Type Definitions</h3>`;
            // Split HTML by Each named entry
            let contentSplitArray = splitBy(currentContent, html_reg_findByName, endTerm, htmlTitle);
            foundSignalsAndMethods = contentSplitArray[1];
            // console.log("foundSignalsAndMethods", foundSignalsAndMethods)
            // Create a reference to the current content after split and the split functions
            currentContent = contentSplitArray[0]
                                .replace(html_reg_typeDefType, html_reg_typeDefType_replace) // Edit how the typedef type looks
                                .replace(html_reg_typeDefinitonsTitle, ""); // Remove Type Definitions Title to be remade later;
            endTerm = html_reg_findByArticleClose;
            // Grab split Type Definitions
            let contentSplitArrayForTypeDefs = splitBy(currentContent, html_reg_findByName, endTerm, htmlTitle);
            currentContent = contentSplitArrayForTypeDefs[0];
            foundTypeDefinitions = contentSplitArrayForTypeDefs[1];
            // console.log("foundTypeDefinitions", foundTypeDefinitions)
            
        } else {
            endTerm = html_reg_findByArticleClose;
            let contentSplitArray = splitBy(currentContent, html_reg_findByName, endTerm, htmlTitle);
            foundSignalsAndMethods = contentSplitArray[1];
            currentContent = contentSplitArray[0];
        }
        
        // Create references to the split methods and signals
        let processedMethodsSignalsAndTypeDefs = splitMethodsSignals(foundSignalsAndMethods);
        let splitMethods = processedMethodsSignalsAndTypeDefs[0];
        let splitSignals = processedMethodsSignalsAndTypeDefs[1];
        let splitTypeDefinitionIDS;
        let splitMethodIDS = extractIDs(splitMethods);
        let splitSignalIDS = extractIDs(splitSignals);
        if (foundTypeDefinitions){
            splitTypeDefinitionIDS = extractIDs(foundTypeDefinitions);
        }
        let arrayToPassToClassToc = [];

        if (splitMethods.length > 0) {
            arrayToPassToClassToc.push({type: "Methods", array: splitMethodIDS});            
            // Add the Methods header to the Methods HTML
            splitMethods.unshift(html_reg_findByMethod_replace)
            currentContent = append(currentContent, html_reg_findByArticleClose, splitMethods.join('\n'), true);
        }
        if (splitSignals.length > 0) {
            arrayToPassToClassToc.push({type: "Signals", array: splitSignalIDS});
            // Add the Signals header to the Signals HTML
            splitSignals.unshift(html_reg_signalTitle)
            currentContent = append(currentContent, html_reg_findByArticleClose, splitSignals.join('\n'),true);        
        }
        if (foundTypeDefinitions && foundTypeDefinitions.length > 0) {
            arrayToPassToClassToc.push({type: "Type Definitions", array: splitTypeDefinitionIDS});                
            // Add the Type Defs header to the Type Defs HTML
            foundTypeDefinitions.unshift(html_reg_typeDefinitonsTitle_replace)
            currentContent = append(currentContent, html_reg_findByArticleClose, foundTypeDefinitions.join('\n'), true);        
        }

        let classTOC = makeClassTOC(arrayToPassToClassToc);
        if (groupName === "Global"){
            currentContent = append(currentContent, html_reg_findByTitle, classTOC);         
        } else if (htmlTitle === "Controller") {
            let curatedList = currentContent.match(html_reg_findControllerCuratedList);
            currentContent = currentContent.replace(html_reg_findControllerCuratedList, "");
            let entityMethods = currentContent.match(html_reg_findEntityMethods);
            currentContent = currentContent.replace(html_reg_findEntityMethods, "");
            currentContent = append(currentContent, html_reg_firstDivClose, [classTOC, curatedList, entityMethods].join("\n"));
            currentContent = currentContent.replace(html_reg_EntityMethodsHeader, html_reg_EntityMethodsHeader_replace);
        } else {
            currentContent = append(currentContent, html_reg_firstDivClose, classTOC);    
        }
        
        // Final Pretty Content
        currentContent = htmlclean(currentContent);
        currentContent = pretty(currentContent);
    
        // Handle Unique Categories
            switch(groupName){
                case "Namespace":
                    handleNamespace(htmlTitle, currentContent);
                    break;
                case "Class":
                    handleClass(htmlTitle, currentContent);
                    break;
                case "Global":
                    handleGlobal(htmlTitle, currentContent);
                    break;
                default:
                    console.log(`Case not handled for ${groupName}`);
            }
    }
})

// Create the base Templates after processing individual files
createTemplate("API-Reference", makeGroupTOC(["Namespaces", "Objects", "Globals"]));
createTemplate("Namespaces", makeGroupTOC("Namespaces"));
createTemplate("Objects", makeGroupTOC("Objects"));

// Copy the files to the target Directories if Local
if (copyLocal){
    // Copy files to the Twig Directory
    let templateFiles = fs.readdirSync(path.resolve(targetTemplateDirectory));
    // Remove Existing API files
    templateFiles.forEach(function(file){
        let curSource = path.join(targetTemplateDirectory, file);
            
        if(path.basename(file, '.html').indexOf("API") > -1){
            fs.unlink(curSource);
        }
        
    })
    copyFolderRecursiveSync(dir_template, targetTemplateDirectory);

    // Copy files to the Md Directory
    let baseMdRefDir = path.join(targetMDDirectory,"08.api-reference");
    // Remove existing MD directory
    if (fs.existsSync(baseMdRefDir)){
        rimraf.sync(baseMdRefDir);
    }
    copyFolderRecursiveSync(dir_md, targetMDDirectory);
}