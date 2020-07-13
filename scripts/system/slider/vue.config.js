module.exports = {
  "transpileDependencies": [
    "vuetify"
],
    pages: {
        'index': {
            entry: './src/main.js',
            template: 'public/index.html',
            title: 'Controller',
            chunks: [ 'chunk-vendors', 'chunk-common', 'index' ]
        },
        'display': {
            entry: './src/main_display.js',
            template: 'public/index.html',
            title: 'Display',
            chunks: [ 'chunk-vendors', 'chunk-common', 'display' ]
        }
    }
}