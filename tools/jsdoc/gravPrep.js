const htmlclean = require('htmlclean');
const fs = require('fs');
const path = require('path');
const pretty = require('pretty');
const cheerio = require('cheerio');
const rimraf = require('rimraf');

// required directories
let dir_out = path.join(__dirname, 'out');

let dir_grav = path.join(dir_out, 'grav');
let dir_css = path.join(dir_grav, 'css');
let dir_js = path.join(dir_grav, 'js');
let dir_template = path.join(dir_grav, 'templates');

let dir_md = path.join(dir_grav, '06.api-reference');
let dir_md_objects = path.join(dir_md, '02.Objects');
let dir_md_namespaces = path.join(dir_md, '01.Namespaces');
let dir_md_globals = path.join(dir_md, '03.Globals');

// maps for sorting
let map_dir_md = {
    "Class": dir_md_objects,
    "Namespace": dir_md_namespaces,
    "Global": dir_md_globals,
}

// html variables to be replaced
const html_reg_static = /<span class="type-signature">\(static\)<\/span>/g
const html_reg_title = /\<h1.+?\>.+?\<\/h1\>/g;
const html_reg_htmlExt = /\.html/g;

// remove grav directory if exists to make sure old files aren't kept
if (fs.existsSync(dir_grav)){
    console.log("dir_grav exists");
    rimraf.sync(dir_grav);
}

// array to itterate over and create if doesn't exist
let dirArray = [dir_grav, dir_css, dir_js, dir_template, dir_md, dir_md_objects, dir_md_namespaces, dir_md_globals];

dirArray.forEach(function(dir){
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir);
    }
})

function createMD(title, directory){
    let mdSource = makeMdSource(title);
    let destinationMDFile = path.join(directory, `API_${title}.md`);
    fs.writeFileSync(destinationMDFile, mdSource);
}

function createTemplate(title,directory, content ){
    let twigBasePartial = makeTwigFile(content);
    let destinationFile = path.join(directory, `API_${title}.html.twig`);
    fs.writeFileSync(destinationFile, twigBasePartial);
}

createMD("API-Reference", dir_md);
createTemplate("API-Reference", dir_template,"");
createMD("Globals", dir_md_globals);
createTemplate("Globals", dir_template,"");
createMD("Namespaces", dir_md_namespaces);
createTemplate("Namespaces", dir_template,"");
createMD("Objects", dir_md_objects);
createTemplate("Objects", dir_template,"");

// read jsdoc output folder

let files = fs.readdirSync(dir_out);
files.forEach(function (file){
    let curSource = path.join(dir_out, file);
    if (path.extname(curSource) == ".html" && path.basename(curSource, '.html') !== "index") {
        // clean up the html source

        let loadedHtml = prepareHtml(curSource);

        // extract the title, groupename, and the main div

        let splitTitle = loadedHtml("title").text().split(": ");
        // console.log(splitTitle);
        let groupName = splitTitle[1];
        let htmlTitle = splitTitle.pop();
        // strip out undesired regex
        let mainDiv = loadedHtml("#main").html();
        let mainDivRegexed = mainDiv.replace(html_reg_static,"")
                                    .replace(html_reg_title,"")
                                    .replace(html_reg_htmlExt,"")

        // create the .md file and corresponding folder

        if (htmlTitle !== "Global"){
            let mdSource = makeMdSource(htmlTitle);
            let destinationDirectory = path.join(map_dir_md[groupName], htmlTitle);
            if (!fs.existsSync(destinationDirectory)) {
                fs.mkdirSync(destinationDirectory);
            }
            let destinationMDFile = path.join(destinationDirectory, `API_${htmlTitle}.md`);
            fs.writeFileSync(destinationMDFile, mdSource);
        } else {
            let mdSource = makeMdSource(htmlTitle);
            let destinationMDFile = path.join(map_dir_md[groupName], `API_Globals.md`);
            fs.writeFileSync(destinationMDFile, mdSource);
        }


        // create the twig template

        let twigBasePartial = makeTwigFile(mainDivRegexed);
        let destinationFile = path.join(dir_template, `API_${htmlTitle}.html.twig`);
        fs.writeFileSync(destinationFile, twigBasePartial);
    }
})

// let curSource = path.join(dir_out, "Camera.html");



function prepareHtml(source){
    let htmlBefore = fs.readFileSync(source, {encoding: 'utf8'});
    let htmlAfter = htmlclean(htmlBefore);
    let htmlAfterPretty = pretty(htmlAfter);
    return cheerio.load(htmlAfterPretty);
}

function makeMdSource(title){
    return (
`---
title: '${title}'
taxonomy:
    category:
        - docs
visible: true
---
`
    )
}

function makeTwigFile(contentHtml){
    return (
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

let targertTwigDirectory = "D:/ROLC/Organize/O_Projects/Hifi/Docs/hifi-docs-grav/user/themes/learn2/";
let targetMDDirectory = "D:/ROLC/Organize/O_Projects/Hifi/Docs/hifi-docs-grav-content/";

// Copy files to the Twig Directory
let templateFiles = fs.readdirSync(path.resolve(targertTwigDirectory));
templateFiles.forEach(function(file){
    let curSource = path.join(targertTwigDirectory, file);    
    if(path.basename(file, '.html').indexOf("API") > -1){
        fs.unlink(curSource);
    }
})

copyFolderRecursiveSync(dir_template, targertTwigDirectory);

// Copy files to the Md Directory

let baseMdRefDir = path.join(targetMDDirectory,"06.api-reference");

if (fs.existsSync(baseMdRefDir)){
    rimraf.sync(baseMdRefDir);
}

copyFolderRecursiveSync(dir_md, targetMDDirectory);

// helper functions
function copyFileSync( source, target ) {
    console.log("sourece:" + source);
    let targetFile = target;

    //if target is a directory a new file with the same name will be created
    if ( fs.existsSync( target ) ) {
        console.log("target exists");
        if ( fs.lstatSync( target ).isDirectory() ) {
            console.log("target is a directory");
            
            targetFile = path.join( target, path.basename( source ) );
            console.log("targetFile:" + targetFile);
            
        }
    }

    fs.writeFileSync(targetFile, fs.readFileSync(source));
}

function copyFolderRecursiveSync( source, target ) {
    var files = [];

    //check if folder needs to be created or integrated
    // console.log("target:" + target)
    // console.log("source:" + source)
    // console.log("basename source:" +  path.basename( source ))
    
    var targetFolder = path.join( target, path.basename( source ) );
    // console.log("targetFolder:" + targetFolder);
    if ( !fs.existsSync( targetFolder ) ) {
        fs.mkdirSync( targetFolder );
    }

    //copy
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
