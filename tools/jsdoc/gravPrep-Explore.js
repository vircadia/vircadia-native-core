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
    let targetTemplateDirectory = ''
    let targetMDDirectory = ''
    if (copyLocal){
        targetTemplateDirectory = process.argv[3];
        targetMDDirectory = process.argv[4];;
    }

// Required directories
    let dir_out = path.join(__dirname, 'out');

    let dir_grav = path.join(dir_out, 'grav');
    let dir_css = path.join(dir_grav, 'css');
    let dir_js = path.join(dir_grav, 'js');
    let dir_template = path.join(dir_grav, 'templates');

    let dir_md = path.join(dir_grav, '06.api-reference');
    let dir_md_objects = path.join(dir_md, '02.Objects');
    let dir_md_namespaces = path.join(dir_md, '01.Namespaces');
    let dir_md_globals = path.join(dir_md, '03.Globals');
    
// Array to itterate over and create if doesn't exist
    let dirArray = [dir_grav, dir_css, dir_js, dir_template, dir_md, dir_md_objects, dir_md_namespaces, dir_md_globals];

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

// Base Grouping Directories for MD files
    let baseMDDirectories = ["API-Reference", "Globals", "Namespaces", "Objects"]

// Html variables to be handle regex replacements
    const html_reg_static = /<span class="type-signature">\(static\)<\/span>/g
    const html_reg_title = /\<h1.+?\>.+?\<\/h1\>/g;
    const html_reg_htmlExt = /\.html/g;
    const html_reg_objectHeader = /<header>[\s\S]+?<\/header>/;
    const html_reg_objectSpanNew = /<h4 class="name"[\s\S]+?<\/span><\/h4>/;
    const html_reg_brRemove = /<br>[\s\S]+?<br>/;
    const html_reg_subsectionEdit = /(<h. class="subsection-title">)([\s\S]*?)(<\/h.>)/g;
    const html_reg_subsectionEdit_replace = '<h4 class="subsection-title">$2</h4>';
    const html_reg_propertiesHeaderEdit = '<h4 class="subsection-title">Properties:</h4>';
    const html_reg_propertiesHeaderEdit_Replace = '<h4 class="subsection-title">Properties</h4>';
    const html_reg_typeEdit = /(<h5>Returns[\s\S]*?Type)(<\/dt[\s\S]*?type">)(.*?)(<\/span><\/dd>[\s\S]*?<\/dl>)/g;
    const html_reg_typeEdit_replace = '$1: $3</dt></dl>'
    const html_reg_methodSize = /(<h4)( class="name"[\s\S].*?<\/span>)(<\/h4>)/g;
    const html_reg_methodSize_replace = '<h5$2</h5>';
    const html_reg_typeDefSize = /(<h4)( class="name"[\s\S].*?)(<\/h4>)/g;
    const html_reg_typeDefSize_replace = '<h5$2</h5>';
    const html_reg_returnSize = /<h5>Returns:<\/h5>/g;
    const html_reg_returnSize_replace = '<h6>Returns:<\/h6>';
    const html_reg_findByName = '<h5 class="name"';
    const html_reg_findByTitle = '<h1>';   
    const html_reg_findByMethod = `<h4 class="subsection-title">Methods</h4>`;
    const html_reg_findByArticleClose = `</article>`
    const html_reg_signalTitle = `<h4 class="subsection-title">Signals</h4>`;
    const html_reg_typeDefinitonsTitle = `<h4 class="subsection-title">Type Definitions</h4>`;

// Mapping for GroupNames and Members
    let groupNameMemberMap = {
        "Objects": [],
        "Namespaces": [],
        "Globals": []
    }

// Procedural functions
    // Create the actual MD file
    function createMD(title, directory, needsDir, isGlobal){
        let mdSource = makeMdSource(title);
        
        if (needsDir){
            if (!fs.existsSync(directory)) {
                fs.mkdirSync(directory);
            }
        }

        let destinationMDFile = path.join(directory, `API_${title}.md`);        
        fs.writeFileSync(destinationMDFile, mdSource);
    }

    // Create the actual Template file
    function createTemplate(title,content){
        let twigBasePartial = makeTwigFile(content);
        let destinationFile = path.join(dir_template, `API_${title}.html.twig`);
        fs.writeFileSync(destinationFile, twigBasePartial);
    }

    // Copy file from source to target - used for recurssive call
    function copyFileSync( source, target ) {
        let targetFile = target;

        // If target is a directory a new file with the same name will be created
        if ( fs.existsSync( target ) ) {
            // console.log("target exists");
            if ( fs.lstatSync( target ).isDirectory() ) {
                // console.log("target is a directory");
                
                targetFile = path.join( target, path.basename( source ) );
            }
        }

        fs.writeFileSync(targetFile, fs.readFileSync(source));
    }

    // Copy file from source to target
    function copyFolderRecursiveSync( source, target ) {
        var files = [];

        // Check if folder needs to be created or integrated
        var targetFolder = path.join( target, path.basename( source ) );
        if ( !fs.existsSync( targetFolder ) ) {
            fs.mkdirSync( targetFolder );
        }

        // Copy
        if ( fs.lstatSync( source ).isDirectory() ) {
            files = fs.readdirSync( source );
            files.forEach( function ( file ) {
                var curSource = path.join( source, file );
                if ( fs.lstatSync( curSource ).isDirectory() ) {
                    copyFolderRecursiveSync( curSource, targetFolder );
                } else {
                    copyFileSync( curSource, targetFolder );
                }
            } );
        }
    }

    // Clean up the Html
    function prepareHtml(source){
        let htmlBefore = fs.readFileSync(source, {encoding: 'utf8'});
        let htmlAfter = htmlclean(htmlBefore);
        let htmlAfterPretty = pretty(htmlAfter);
        return cheerio.load(htmlAfterPretty);
    }

    // Base file for MD's
    function makeMdSource(title){
        return dedent(
            `
            ---
            title: ${title}
            taxonomy:
                category:
                    - docs
            visible: true
            ---
            `
        )
    }

    // Base file for Templates
    function makeTwigFile(contentHtml){
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
                <div id="body-inner">
                <h1>{{ page.title }}</h1>
                ${contentHtml}
                </div>
            {% endblock %}
            `
        )
    }

    // Handle NameSpace Group   
    function handleNamespace(title, content){
        groupNameMemberMap["Namespaces"].push(title);
        let destinationDirectory = path.join(map_dir_md["Namespace"], title);
        createMD(title, destinationDirectory, true);
        createTemplate(title, content);
    }

    // Handle Class Group       
    function handleClass(title, content){
        groupNameMemberMap["Objects"].push(title);
        let destinationDirectory = path.join(map_dir_md["Class"], title);
        createMD(title, destinationDirectory, true)

        let formatedHtml = content
                            .replace(html_reg_objectSpanNew,"")
        createTemplate(title, formatedHtml);
    }

    // Handle Global Group           
    function handleGlobal(title, content){
        groupNameMemberMap["Globals"].push("Globals");
        createMD("Globals", map_dir_md["Global"], false, true);
        createTemplate("Globals", content); 
    }

    // Handle Group TOCs               
    function makeGroupTOC(group){
        let mappedGroup;
        if (!Array.isArray(group)){
            mappedGroup = groupNameMemberMap[group];
        } else {
            mappedGroup = group;
        }
        let htmlGroup = mappedGroup.map( item => {
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
               linkArray.push(`<div><a href="#.${link.slice(1)}">${link.slice(1)}</a></div>`)
            })
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

            }
            extractedIDs.push(id)
        })
        return extractedIDs;
    }

    // Helper for splitting up html 
    // Takes: Content to split, SearchTerm to Split by, term to End Splitting By, and negate Term object array
    // negateTermObject { term: "<code>", distance: "3"}
    // Returns: [newContent after Split, Array of extracted ]
    function splitBy(content, searchTerm, endSplitTerm){
        let foundArray = [];
        let curIndex = -1;
        let afterCurSearchIndex = -1
        let negateTermIndex = -1;
        let nextIndex = 0;
        let findbyNameLength = searchTerm.length;
        let curfoundArrayIndex = 0;
        let curEndSplitTermIndex = -1;
        do {
            curEndSplitTermIndex = content.indexOf(endSplitTerm);
            curIndex = content.indexOf(searchTerm);
            afterCurSearchIndex = curIndex+findbyNameLength;
            nextIndex = content.indexOf(searchTerm,afterCurSearchIndex);
            if (nextIndex === -1){
                nextIndex = curEndSplitTermIndex;
            }
            foundArray.push(content.slice(curIndex, nextIndex))
            // remove that content
            content = content.replace(foundArray[curfoundArrayIndex], "");
            curfoundArrayIndex++;
        } while (curIndex > -1)
        return [content, foundArray];
    }

    // Split the signals, methods, and typeDefs [Might make this more generic]
    function splitMethodsSignalsAndTypeDefs(allItemToSplit){
        let methodArray = [];
        let signalArray = [];
        let typeDefArray = [];
        console.log(allItemToSplit.length);
        allItemToSplit.forEach( method => {
            firstLine = method.split("\n")[0];            
            if (firstLine.indexOf("Signal") > -1){
                console.log("Found signal")
                signalArray.push(method);
            } else if (firstLine.indexOf("span") > -1) {
                // console.log("Found method")
                methodArray.push(method);
            } else {
                // console.log("Found typeDef")
                
                typeDefArray.push(method);
            }
        })
        return [methodArray, signalArray, typeDefArray];
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
    let curSource = path.join(dir_out, "Controller.html");
    if (path.extname(curSource) == ".html" && path.basename(curSource, '.html') !== "index") {
        // Clean up the html source
            let loadedHtml = prepareHtml(curSource);

        // Extract the title, group name, and the main div
            let splitTitle = loadedHtml("title").text().split(": ");
            let groupName = splitTitle[1];
            let htmlTitle = splitTitle.pop();
            let mainDiv = loadedHtml("#main")

            let methodIDs = [];
            let signalIDs = [];
            let typeDefIDs = [];
        // Basic Regex HTML edits
            let mainDivRegexed = mainDiv.html()
                                    .replace(html_reg_static,"")
                                    .replace(html_reg_title,"")
                                    .replace(html_reg_objectHeader,"")
                                    .replace(html_reg_htmlExt,"")
                                    .replace(html_reg_brRemove, "")
                                    .replace(html_reg_subsectionEdit, html_reg_subsectionEdit_replace)
                                    .replace(html_reg_propertiesHeaderEdit, html_reg_propertiesHeaderEdit_Replace)
                                    .replace(html_reg_typeEdit, html_reg_typeEdit_replace)
                                    .replace(html_reg_returnSize, html_reg_returnSize_replace)
                                    .replace(html_reg_methodSize, html_reg_methodSize_replace)
                                    .replace(html_reg_typeDefSize, html_reg_typeDefSize_replace)
                                    .replace(html_reg_typeDefinitonsTitle, "");
                                    
        
        // Further HTML Manipulation
            // Split HTML by Each named entry
            let contentSplitArray = splitBy(mainDivRegexed, html_reg_findByName, html_reg_findByArticleClose);
            // Create a reference to the current content after split and the split functions
            let currentContent = contentSplitArray[0];
            // Create references to the split methods and signals
            let splitMethodsSignalsAndTypeDefs = splitMethodsSignalsAndTypeDefs(contentSplitArray[1]);
            let splitMethods = splitMethodsSignalsAndTypeDefs[0];
            let splitSignals = splitMethodsSignalsAndTypeDefs[1];
            let splitTypeDefintions = splitMethodsSignalsAndTypeDefs[2];
            let splitMethodIDS = extractIDs(splitMethods);
            let splitSignalIDS = extractIDs(splitSignals);
            let splitTypeDefinitionIDS = extractIDs(splitTypeDefintions);
            let classTOC = makeClassTOC([
                {type: "Methods", array: splitMethodIDS},
                {type: "Signals", array: splitSignalIDS},
                {type: "Type Definitions", array: splitTypeDefinitionIDS}
            ]);

            // Append Signals and Methods to the current Content
            currentContent = append(currentContent, html_reg_findByTitle, classTOC);         
            currentContent = append(currentContent, html_reg_findByMethod, splitMethods.join('\n'));
            if (splitSignals.length > 0) {
                // Add the Signals header to the Signals HTML
                splitSignals.unshift(html_reg_signalTitle)
                currentContent = append(currentContent, html_reg_findByArticleClose, splitSignals.join('\n'),true);        
            }
            if (splitTypeDefintions.length > 0) {
                // Add the Signals header to the Signals HTML
                splitTypeDefintions.unshift(html_reg_typeDefinitonsTitle)
                currentContent = append(currentContent, html_reg_findByArticleClose, splitTypeDefintions.join('\n'), true);        
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
    let baseMdRefDir = path.join(targetMDDirectory,"06.api-reference");
    // Remove existing MD directory
    if (fs.existsSync(baseMdRefDir)){
        rimraf.sync(baseMdRefDir);
    }
    copyFolderRecursiveSync(dir_md, targetMDDirectory);
}
