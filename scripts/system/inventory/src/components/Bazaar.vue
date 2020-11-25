<!--
//  Bazaar.vue
//
//  Created by Kalila L. on 10 November 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
-->

<template>
    <v-container fluid>
        <v-col
            cols="12"
            sm="6"
            md="4"
            lg="3"
        >
            
        </v-col>
    </v-container>
</template>

<script>
import { EventBus } from '../plugins/event-bus.js';
// eslint-disable-next-line
var inventoryDB = new PouchDB('inventory');

// Import exported GZ file
let $repo = "http://localhost:8081"
var data = Get($repo + "/js/inventoryDB.gz");

// Unzip import and load into inventoryDB
function mkDB(data) {
    // eslint-disable-next-line
    data = pako.ungzip(data, {
        to: 'string'
    });
    inventoryDB.bulkDocs(
        JSON.parse(data), {
        new_edits: false
    } // not change revision
    );
    console.log('Finished Import');
}

// GET Function to retrieve json
function Get($URL) {
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET", $URL, false);
    Httpreq.send(null);
    return Httpreq.responseText;
}

// Write DB Info to console
inventoryDB.info().then(function (info) {
    var dbName = info.db_name;
    var records = info.doc_count;
    console.info('dbName', dbName, 'records', records);
})


// DB Function to pull top level categories
function getTcat() {
    inventoryDB.search({
        query: 'Bazzar',
        fields: ['parent'],
        include_docs: true
    }).then(function (res) {
        console.info('GetTCats', res);
        // inventoryDB.search({
        //     query: 'Avatars',
        //     fields: ['name'],
        //     include_docs: true
        // }).then(function (data) {
        //     console.info('Avatars', data)
        // });
        inventoryDB.search({
            query: 'Avatars',
            fields: ['parent'],
            include_docs: true
        }).then(function (data) {
            console.info('Avatars', data);
        });
    });
}

// // DB Function to get sub-categories from a given Category
// function getSub($sub, $cat) {
//     if (typeof $sub === 'object') {
//         $sub = $sub.docs[0].name
//     };
//     var subCatDiv = document.getElementById('tLevel');
//     // Find all folders who call this parent
//     inventoryDB.search({ //Using simple search
//         query: $sub,
//         fields: ['parent'],
//         include_docs: true
//     }).then(function (res) {
//         // Send to print as child and check for sub folders
//         $length = res.total_rows;
//         //        while ($i < $length) {
//         for (const key of Object.keys(res)) {
//             if (key != "total_rows") {
//                 $rec = res[key];
//                 for (const item of Object.keys($rec)) {                   
//                     if ($rec[item].doc.r_type == "folder") {
// 						$parent = $rec[item].doc.parent;
// 						$lparent = $parent.replace(/-/g, "_");
// 						$lparent = $lparent.replace(/'/g, "_");
// 						$name = $rec[item].doc.name;
// 						$pname = $name.replace(/_/g, " ");						
// 						$pname = $pname[0].toUpperCase() + $name.substr(1);
// 						$pname = $pname.replace(/_/g, " ");						
// 						$lname = "f_" + $name + "_nested";							
// 						$id = $rec[item].doc._id;
// 						if ($parent == $cat){$folderUL = "folderUL"}else{$folderUL = "f_"+$parent+"_nested"};
//                         document.getElementById($folderUL).innerHTML += "<li id='" + $id + "'><span class='caret'>" + $pname + "</span><ul id='" + $lname + "' class='nested'><ul>";
// 	                    printTree($name, $cat)
//                     };						
//                 }
//             }
//         };
//     });
//     document.getElementById('folderUL').innerHTML += "</li>";
// 
//     // Toggle Tree
//     var toggler = document.getElementsByClassName("caret");
//     var i;
// 
//     for (i = 0; i < toggler.length; i++) {
//         toggler[i].addEventListener("click", function () {
//             this.parentElement.querySelector(".nested").classList.toggle("active");
//             this.classList.toggle("caret-down");
//         }, true);
//     }
// 
// };
// 
// function printTree(res2, $cat) {
//     $length = res2.total_rows;
//     inventoryDB.search({
//         query: res2,
//         fields: ['name'],
//         include_docs: true
//     }).then(function (data) {
//         // Send to print as child and check for sub folders
//         for (const key1 of Object.keys(data)) {
//             if (key1 != "total_rows") {
//                 $rec1 = data[key1];
//                 for (const item of Object.keys($rec1)) {
//                     var $path = $rec1[item].doc.path.match(/.*\/(.*)\/(.*)$/);
//                     var $parent = $rec1[item].doc.parent;
//                     if ($rec1[item].doc.r_type == "resource") {
//                         $name = $rec1[item].doc.name;
//                         $id = $rec1[item].doc._id;
//                         $pname = $name[0].toUpperCase() + $name.substr(1);
//                         $parent = "f_" + $name + "_nested";
//                         var $path = $rec1[item].doc.path;
//                         console.log($path)
//                         // Get the model JSON and parse it...
//                         $modelData = Get($path + "/resource.json");
//                         $model = JSON.parse($modelData)
//                             $description = $model.description;
//                         $author = $model.author.name;
//                         $license = $model.author.license;
//                         $sublicense = $model.sublicense;
// 						$subs = "";
//                         for ($a = 0; $a < $sublicense.length; $a++) {
//                             var $slicense = $sublicense[$a].license;
//                             var $sname = $sublicense[$a].name;
// 
//                             if (!$sname.includes("undefined")) {
//                                 var $subs = $slicense + " " + $sname + " | " + $subs;
//                             }
//                         }
//                         // icon.png is only 90px wide so we bump it up
//                         document.getElementById($parent).innerHTML += "<table><tr><td style='padding: 15px;vertical-align: top;'><img width='150px' height='auto' src = '" + $path + "/img/icon.png'></td><td style='padding: 15px;vertical-align: top;'>" + $description + "<br>" + $license + " " + $author + "</td></tr><tr><td colspan='2'><p>" + $subs + "</p></td></tr></table>";
//                         $subs = "";
//                     };
// 
//                 }
//             }
//         };
//     });
//     getSub(res2, $cat)
// };

// // Create top level tabs
// function printCats(res) {
//     var topCatDiv = document.getElementById('tcat');
// 	var subCatDiv = document.getElementById('dump');
// 	$length = res.total_rows;	
//     $i = 0;
//     while ($i < $length) {
//         $cat = res.rows[$i].doc.name;
//         $Cat = $cat[0].toUpperCase() + $cat.substr(1);
//         $cTab = $cat + $i;
//         // Add tabs
//         topCatDiv.innerHTML += "<button id=\"" + $cTab + "\" class=\"tablinks\" onclick=\"openTab(event, '" + $Cat + "')\">" + $Cat + "</button>";
//         // Build blocks for content
//         subCatDiv.innerHTML += "<div id = \"" + $Cat + "\" class=\"tabcontent\">Do something with " + $Cat + " link...</div>";
//         // Set first tab as activ tab and display block
//         if ($i == 0) {
//             document.getElementById($Cat).style.display = "block";
//             document.getElementById($cTab).className += " active";
//         }
//         $i = $i + 1;
//     };
// };
// 
// // Tab Support
// function openTab(evt, tabName) {
//     // Declare all variables
//     var i,
//     tabcontent,
//     tablinks;
// 
//     // Get all elements with class="tabcontent" and hide them
//     tabcontent = document.getElementsByClassName("tabcontent");
//     for (i = 0; i < tabcontent.length; i++) {
//         tabcontent[i].style.display = "none";
//     }
// 
//     // Get all elements with class="tablinks" and remove the class "active"
//     tablinks = document.getElementsByClassName("tablinks");
//     for (i = 0; i < tablinks.length; i++) {
//         tablinks[i].className = tablinks[i].className.replace(" active", "");
//     }
// 
//     // Show the current tab, and add an "active" class to the button that opened the tab
//     document.getElementById(tabName).style.display = "block";
//     evt.currentTarget.className += " active";
// 
// 	// Reset Category Tree ...
// 	document.getElementById('folderUL').innerHTML = "";
// 	$cat = tabName;
// 	getSub(tabName, $cat); 
// 	document.getElementById($cTab).className += " active";
// }

// // Get all models under parent (Category)
// function getAvatars(){
//     inventoryDB.search({
//         query: 'Avatars',
//         fields: ['parent'],
//         include_docs: true
//     }).then(function (data) {
// 	  console.info('Avatars', data);
// 	});
// }
// 
// // Display avatars
// function displayAvatars(data){
//     $length = data.total_rows;
// 	$i = 0;
// 	while ($i < $length) {
// 	  $aurl = data.rows[$i].doc.path;
// 	  $apath = data.rows[$i].doc.path + "/resource.json";
// 	  $adata = Get($apath);
// 	  if (!$adata.includes("<Error>")){
// 		$aresource = JSON.parse($adata);
// 		$aname = $aresource.name;
// 		$adescription = $aresource.description;
// 		$aversion = $aresource.version;
// 		$apic = $aurl + "/" + $aresource.images;
// 		$aauthor = $aresource.author.name;
// 		$alicense = $aresource.author.license;
// 		$asublicense = $aresource.sublicense;
// 		$acontributor = $aresource.contributors.name;
// 		$amain = $aurl + "/" +$aresource.main;
// 		var $subs;
// 		for ($a = 0; $a < $asublicense.length; $a++) {
// 		  console.log(JSON.stringify($asublicense[$a]))
// 		  var $slicense = $asublicense[$a].license;
// 		  var $sname = $asublicense[$a].name;
// 		  if($sname != "undefined"){
// 			$subs = $slicense + " " + $sname + " | " + $subs;
// 		  }
// 		}
// document.getElementById("avatars").innerHTML += "<span style='display: inline-block; border: 2px solid black;' width='500px'><table border='0' width='500px'><tr><td style='padding: 15px;vertical-align: top;'><center><img width='90%' height='auto' src = '"+$apic+"'></center></td></tr><tr><td style='padding: 15px;vertical-align: top;'><a href='"+$amain+"'>"+$aname+"</a> version: "+$aversion+" - "+$adescription+"<br>"+$alicense+" "+$aauthor+"</td></tr><tr><td style='padding: 15px;vertical-align: top;'>"+$subs+"</td></tr></table></span><br><br>";
// 		$subs = "";
// 	  }
// 	  $i++;
// 	};
// };

// Call function to load DB
mkDB(data);
// Call to create top level categories
getTcat();
// Call to display Avatars
// getAvatars();

export default {
    name: 'Bazaar',
    components: {
    },
    props: [],
    data: () => ({

    }),
    created: function () {
        console.log('eventBus' + EventBus);
    },
    computed: {
        
    },
    watch: {
        
    },
    methods: {
        
    }
};
</script>