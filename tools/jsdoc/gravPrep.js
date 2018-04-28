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
let dir_twig = path.join(dir_grav, 'templates');

let dir_md = path.join(dir_grav, '06.api-reference');
let dir_md_classes = path.join(dir_md, 'Objects');
let dir_md_namespaces = path.join(dir_md, 'Namespaces');
let dir_md_globals = path.join(dir_md, 'Globals');

// maps for sorting
let map_dir_md = {
    "Class": dir_md_classes,
    "Namespace": dir_md_namespaces,
    "Global": dir_md_globals
}

// html variables to be replaced
const html_reg_static = /\<span class="type-signature"\>(static)\<\/span>/g;
const html_reg_title = /\<h1.+?\>.+?\<\/h1\>/g;
const html_reg_htmlExt = /\.html/g;

// remove grav directory if exists to make sure old files aren't kept
if (fs.existsSync(dir_grav)){
    console.log("dir_grav exists");
    rimraf.sync(dir_grav);
}

// array to itterate over and create if doesn't exist
let dirArray = [dir_grav, dir_css, dir_js, dir_twig, dir_md, dir_md_classes, dir_md_namespaces, dir_md_globals];

dirArray.forEach(function(dir){
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir);
    }
})

// read jsdoc output folder

/*

let files = fs.readdirSync(dir_out);
files.forEach(function (file){
    let curSource = path.join(dir_out, file);
    if (path.extname(curSource) == ".html") {

        // clean up the html source

        let loadedHtml = prepareHtml(curSource);

        // extract the title and the main div

        let splitTile = loadedHtml("title").text().split(": ");
        let groupName = splitTitle[0];
        let htmlTitle = splitTile.pop();
        console.log(groupName);
        // let mainDiv = loadedHtml("#main").html();
        // let mainDivNoTitle = mainDiv.replace(/\<h1.+?\>.+?\<\/h1\>/g, "");
        // let mainDivStripLinks = mainDivNoTitle.replace(/\.html/g, "");

        // create the .md file and corresponding folder

        // let mdSource = makeMdSource(htmlTitle);
        // let destinationDirectory = path.join(dir_md, htmlTitle);
        // if (!fs.existsSync(destinationDirectory)) {
        //     fs.mkdirSync(destinationDirectory);
        // }
        // let destinationMDFile = path.join(destinationDirectory, `API_${htmlTitle}.md`);
        // fs.writeFileSync(destinationMDFile, mdSource);

        // create the twig template

        // let twigBasePartial = makeTwigFile(mainDivStripLinks);
        // let destinationFile = path.join(dir_twig, `API_${htmlTitle}.html.twig`);
        // fs.writeFileSync(destinationFile, twigBasePartial);
    }
})

*/

let curSource = path.join(dir_out, "Camera.html");

// clean up the html source

let loadedHtml = prepareHtml(curSource);

// extract the title, groupename, and the main div

let splitTitle = loadedHtml("title").text().split(": ");
let groupName = splitTitle[1];
let htmlTitle = splitTitle.pop();
console.log("groupname:", groupName);
console.log("htmlTitle:", htmlTitle);

// strip out undesired regex
let mainDiv = loadedHtml("#main").html();
let mainDivRegexed = mainDiv.replace(html_reg_static,"")
                            .replace(html_reg_title,"")
                            .replace(html_reg_htmlExt,"")

// create the .md file and corresponding folder

console.log(map_dir_md[groupName]);
let mdSource = makeMdSource(htmlTitle);
let destinationDirectory = path.join(map_dir_md[groupName], htmlTitle);
if (!fs.existsSync(destinationDirectory)) {
    fs.mkdirSync(destinationDirectory);
}
let destinationMDFile = path.join(destinationDirectory, `API_${htmlTitle}.md`);
fs.writeFileSync(destinationMDFile, mdSource);

// create the twig template

let twigBasePartial = makeTwigFile(mainDivRegexed);
let destinationFile = path.join(dir_twig, `API_${htmlTitle}.html.twig`);
fs.writeFileSync(destinationFile, twigBasePartial);

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

let chapterMD =
`---
title: 'High Fidelity API Reference'
taxonomy:
    category:
        - docs
---

### 

# API Reference

**Under Construction**: We're currently working on creating an API Reference where you can access functions and events easily. 

Check out our latest API Reference here. We're doing our best to keep this reference up-to-date with each release. If you don't find information you are looking for, contact us at [docs@highfidelity.io](mailto:docs@highfidelity.io). 

`

// Copy files to the Twig Directory
let files = fs.readdirSync(path.resolve(targertTwigDirectory));
files.forEach(function(file){
    let curSource = path.join(targertTwigDirectory, file);    
    if(path.basename(file, '.html').indexOf("API") > -1){
        fs.unlink(curSource);
    }
})

copyFolderRecursiveSync(dir_twig, targertTwigDirectory);

// Copy files to the Md Directory

let baseMdRefDir = path.join(targetMDDirectory,"06.api-reference");

if (fs.existsSync(targetMDDirectory)){
    rimraf.sync(baseMdRefDir);
}

copyFolderRecursiveSync(dir_md, targetMDDirectory);
let chapterDestinationFile = path.join(baseMdRefDir, `chapter.md`);
fs.writeFileSync(chapterDestinationFile, chapterMD);


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
    console.log("target:" + target)
    console.log("source:" + source)
    console.log("basename source:" +  path.basename( source ))
    
    var targetFolder = path.join( target, path.basename( source ) );
    console.log("targetFolder:" + targetFolder);
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
