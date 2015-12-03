var gulp = require('gulp');
var exec = require('child_process').exec;

gulp.task('build', function() {
    exec('npm run build', function(msg){
        console.log(msg);
    });
});

gulp.task('watch', function() {
    gulp.watch('client/*.jsx', ['build']);
});


gulp.task('default', ['build', 'watch'])