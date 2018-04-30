// Dependencies
    const htmlclean = require('htmlclean');
    const fs = require('fs');
    const path = require('path');
    const pretty = require('pretty');
    const cheerio = require('cheerio');
    const rimraf = require('rimraf');
    const dedent = require('dedent-js');

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

// Target Copy Directories
    let targetTemplateDirectory = "D:/ROLC/Organize/O_Projects/Hifi/Docs/hifi-docs-grav/user/themes/learn2/";
    let targetMDDirectory = "D:/ROLC/Organize/O_Projects/Hifi/Docs/hifi-docs-grav-content/";

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
    const html_reg_typeEdit = /(<h5>Returns[\s\S]*?Type)(<\/dt[\s\S]*?type">)(.*?)(<\/span><\/dd>[\s\S]*?<\/dl>)/g;
    const html_reg_typeEdit_replace = '$1: $3</dt></dl>'
    const html_reg_methodSize = /(<h4)( class="name"[\s\S].*?<\/span>)(<\/h4>)/g;
    const html_reg_methodSize_replace = '<h5$2</h5>';
    const html_reg_returnSize = /<h5>Returns:<\/h5>/g;
    const html_reg_returnSize_replace = '<h6>Returns:<\/h6>';
    const html_reg_findByName = '<h5 class="name"';
    const html_reg_findByMethod = `<h4 class="subsection-title">Methods</h4>`
    const html_reg_findByArticleClose = `</article>`
    const html_reg_signalTitle = `<h4 class="subsection-title">Signals</h4>`;
    
    

// Mapping for GroupNames and Members
    let groupNameMemberMap = {
        "Objects": [],
        "Namespaces": [],
        "Globals": []
    }

// Procedural functions
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

    function createTemplate(title,content){
        // console.log("content:", content)
        let twigBasePartial = makeTwigFile(content);
        // console.log("TWIG BASE:", twigBasePartial)
        let destinationFile = path.join(dir_template, `API_${title}.html.twig`);
        fs.writeFileSync(destinationFile, twigBasePartial);
    }

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

    function prepareHtml(source){
        let htmlBefore = fs.readFileSync(source, {encoding: 'utf8'});
        let htmlAfter = htmlclean(htmlBefore);
        let htmlAfterPretty = pretty(htmlAfter);
        return cheerio.load(htmlAfterPretty);
    }

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

    function handleNamespace(title, content){
        // console.log("Content from namespcae:", content)
        groupNameMemberMap["Namespaces"].push(title);
        let destinationDirectory = path.join(map_dir_md["Namespace"], title);

        createMD(title, destinationDirectory, true);
        createTemplate(title, content);
    }

    function handleClass(title, content){
        groupNameMemberMap["Objects"].push(title);
        let destinationDirectory = path.join(map_dir_md["Class"], title);
        createMD(title, destinationDirectory, true)

        let formatedHtml = content
                            .replace(html_reg_objectSpanNew,"")
                            .replace(html_reg_brRemove, "");
        createTemplate(title, formatedHtml);
    }

    function handleGlobal(title, content){
        groupNameMemberMap["Globals"].push("Globals");
        createMD("Globals", map_dir_md["Global"], false, true);
        createTemplate("Globals", content); 
    }

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
    let curSource = path.join(dir_out, "AvatarInputs.html");
        // Clean up the html source
            let loadedHtml = prepareHtml(curSource);

        // Extract the title, group name, and the main div
            let splitTitle = loadedHtml("title").text().split(": ");
            let groupName = splitTitle[1];
            let htmlTitle = splitTitle.pop();
            // console.log("first loaded:", loadedHtml("#main").html())
            let mainDiv = loadedHtml("#main")

            // let signatures = mainDiv.find(".name")
        // Exploring Extractions
            // let array = mainDiv.find('h4').toArray();
            
            // console.log(array[2])
            // var reducedArray = array.reduce((prev, cur) => {
            //     try {
            //         // console.log(cur.children[1]);
            //         prev.push(cur.children[1].data);
            //     } catch(e) {

            //     }
            //     return prev;
            // }, [])
            // console.log("name", reducedArray.length);
            
        // regex edits
            let mainDivRegexed = mainDiv.html()
                                    .replace(html_reg_static,"")
                                    .replace(html_reg_title,"")
                                    .replace(html_reg_objectHeader,"")
                                    .replace(html_reg_htmlExt,"")
                                    .replace(html_reg_brRemove, "")
                                    .replace(html_reg_subsectionEdit, html_reg_subsectionEdit_replace)
                                    .replace(html_reg_typeEdit, html_reg_typeEdit_replace)
                                    .replace(html_reg_returnSize, html_reg_returnSize_replace)
                                    .replace(html_reg_methodSize, html_reg_methodSize_replace);
           
            function splitBy(content, searchTerm){
                let foundArray = [];
                let curIndex = -1;
                let nextIndex = 0;
                let findbyNameLength = searchTerm.length;
                let curfoundArrayIndex = 0;
                let curArticleCloseIndex = -1;
                do {
                    curArticleCloseIndex = content.indexOf("</article>");                    
                    curIndex = content.indexOf(searchTerm);
                    nextIndex = content.indexOf(searchTerm,curIndex+findbyNameLength);
                    if (nextIndex === -1){
                        nextIndex = curArticleCloseIndex
                    }
                    foundArray.push(content.slice(curIndex, nextIndex))
                    content = content.replace(foundArray[curfoundArrayIndex], "");
                    curfoundArrayIndex++;
                } while (curIndex > -1)
                return [content, foundArray];
            }

            function extractSignals(methodArray){
                let newMethodArray = [];
                let signalArray = [];
                methodArray.forEach( method => {
                    method.indexOf("Signal") > -1
                    ? signalArray.push(method)
                    : newMethodArray.push(method);
                })
                return [newMethodArray, signalArray]
            }

            function append(content, areaToAppendTo, contentToAppend, appendBefore){
                var contentArray = content.split("\n");
                // console.log("contentArray:", contentArray)
                // console.log("areaToAppendTo:", areaToAppendTo)
                var foundIndex = findArrayTrim(contentArray, areaToAppendTo)
                console.log(foundIndex)
                foundIndex = appendBefore ? foundIndex : foundIndex +1
                
                contentArray.splice(foundIndex,0,contentToAppend)
                return contentArray.join("\n")
            }

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

            var nameArray = splitBy(mainDivRegexed, html_reg_findByName);
            let currentContent = nameArray[0];
            let separatedMethods = extractSignals(nameArray[1]);
            let methodsToAppend = separatedMethods[0].join("\n");
            separatedMethods[1].unshift(html_reg_signalTitle)
            let signalsToAppend = separatedMethods[1].join("\n");
            currentContent = append(currentContent, html_reg_findByMethod, methodsToAppend);
            mainDivRegexed = append(currentContent, html_reg_findByArticleClose, signalsToAppend,true);        

            // console.log(mainDivRegexed);
        // Handle Unique Categories
            switch(groupName){
                case "Namespace":
                    handleNamespace(htmlTitle, mainDivRegexed);
                    break;
                case "Class":
                    handleClass(htmlTitle, mainDivRegexed);
                    break;
                case "Global":
                    handleGlobal(htmlTitle, mainDivRegexed);
                    break;
                default:
                    console.log(`Case not handled for ${groupName}`);
            }

// Create the base Templates after processing individual files
    createTemplate("API-Reference", makeGroupTOC(["Namespaces", "Objects", "Globals"]));
    createTemplate("Namespaces", makeGroupTOC("Namespaces"));
    createTemplate("Objects", makeGroupTOC("Objects"));

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
