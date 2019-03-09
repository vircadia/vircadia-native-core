# hifi-jsdoc-template
The hifi-jsdoc-template is based on the [DocDash](https://github.com/clenemt/docdash) template. 

## Usage
Clone repository to your designated `jsdoc/node_modules` template directory. 

In your `config.json` file, add a template option.

```json
"opts": {
  "template": "node_modules/hifi-jsdoc-template"
}
```

## Sample `config.json`

```json
{
    "opts": {
        "template": "node_modules/hifi-jsdoc-template"
    },
    "docdash": {
        "meta": {                       
            "title": "",                
            "description": "",          
            "keyword": ""               
        },
        "search": [true],  
        "collapse": [true],       
        "typedefs": [false]    
    },    
    "templates": {
        "default": {
            "outputSourceFiles": false
        }
    },
    "plugins": [
        "plugins/hifi",
        "plugins/hifiJSONExport"
    ]
}
```
