module.exports = function (grunt) {
    //get source files
    var includes = {
        app: require('./app/js/includes/includes.app'),
        turbo: require('./app/js/includes/includes.turbo'),
        turbo_small: require('./app/js/includes/includes.turbo_small')
    }
    //normalize source files:
    // prefix "app" folder to all source files
    // replace config.js with correct one
    var configOption = grunt.option('config');
    function normalizeFilePath(arr){
        for(var ct=0;ct<arr.length;ct++){
            arr[ct] = ['app/', arr[ct]].join('');
            //Replace config file
            if (configOption && arr[ct].indexOf('config.js') != -1){
                var newConfigFile = arr[ct].replace('config.js', ['config/', configOption, '.js'].join(''));
                grunt.log.write('Replacing config ');
                grunt.log.write(arr[ct]['cyan']);
                grunt.log.write(' with ');
                grunt.log.writeln(newConfigFile['cyan']);
                arr[ct] = newConfigFile;
            }
        }
    }
    for (var ctI in includes){
        normalizeFilePath(includes[ctI].lib);
        normalizeFilePath(includes[ctI].js);
    }
    grunt.initConfig({
        pkg: grunt.file.readJSON('package.json'),
        copy: {
            target: {
                files: [
                    {expand: true, cwd: 'app/', src: ['index.html', 'turbo.html', 'turbo_small.html', 'js/includes/includes.js', 'lib/jquery/jquery.js',
                        'forms/**', 'img/**', 'partials/**'].concat(includes.app.css), dest: 'dist/'},
                    {expand: true, cwd: 'app/', src: ['js/includes/includes.app.min.js'], rename: function () {
                        return 'dist/js/includes/includes.app.js';
                    }},
                    {expand: true, cwd: 'app/', src: ['js/includes/includes.turbo.min.js'], rename: function () {
                        return 'dist/js/includes/includes.turbo.js';
                    }},
                    {expand: true, cwd: 'app/', src: ['js/includes/includes.turbo_small.min.js'], rename: function () {
                        return 'dist/js/includes/includes.turbo_small.js';
                    }}
                ]
            }
        },
        concat: {
            app: {
                options: {
                    separator: grunt.util.linefeed
                },
                src: includes.app.js,
                dest: 'dist/js/app.js'
            },
            app_lib: {
                options: {
                    separator: grunt.util.linefeed
                },
                src: includes.app.lib,
                dest: 'dist/js/app_lib.js'
            },
            turbo: {
                options: {
                    separator: grunt.util.linefeed
                },
                src: includes.turbo.js,
                dest: 'dist/js/turbo.js'
            },
            turbo_lib: {
                options: {
                    separator: grunt.util.linefeed
                },
                src: includes.turbo.lib,
                dest: 'dist/js/turbo_lib.js'
            },
            turbo_small: {
                options: {
                    separator: grunt.util.linefeed
                },
                src: includes.turbo_small.js,
                dest: 'dist/js/turbo_small.js'
            },
            turbo_small_lib: {
                options: {
                    separator: grunt.util.linefeed
                },
                src: includes.turbo_small.lib,
                dest: 'dist/js/turbo_small_lib.js'
            }
        },
        ngmin:{
            app: {
                src: ['dist/js/app.js'],
                dest: 'dist/js/app.js'
            },
            turbo:{
                src: ['dist/js/turbo.js'],
                dest: 'dist/js/turbo.js'
            },
            turbo_small:{
                src: ['dist/js/turbo_small.js'],
                dest: 'dist/js/turbo_small.js'
            }
        },
        uglify: {
            app: {
                options: {
                    banner: '/*! <%= pkg.name %> app.js <%= grunt.template.today("dd-mm-yyyy") %> */\n',
                    footer: '//@ sourceURL=app.js',
                    mangle: true
                },
                files: {
                    'dist/js/app.js': ['dist/js/app.js']
                }
            },
            app_lib: {
                options: {
                    banner: '/*! <%= pkg.name %> app_lib.js <%= grunt.template.today("dd-mm-yyyy") %> */\n',
                    footer: '//@ sourceURL=app_lib.js',
                    mangle: true
                },
                files: {
                    'dist/js/app_lib.js': ['dist/js/app_lib.js']
                }
            },
            turbo: {
                options: {
                    banner: '/*! <%= pkg.name %> turbo.js <%= grunt.template.today("dd-mm-yyyy") %> */\n',
                    footer: '//@ sourceURL=turbo.js',
                    mangle: true
                },
                files: {
                    'dist/js/turbo.js': ['dist/js/turbo.js']
                }
            },
            turbo_lib: {
                options: {
                    banner: '/*! <%= pkg.name %> turbo_lib.js <%= grunt.template.today("dd-mm-yyyy") %> */\n',
                    footer: '//@ sourceURL=turbo_lib.js',
                    mangle: true
                },
                files: {
                    'dist/js/turbo_lib.js': ['dist/js/turbo_lib.js']
                }
            },
            turbo_small: {
                options: {
                    banner: '/*! <%= pkg.name %> turbo_small.js <%= grunt.template.today("dd-mm-yyyy") %> */\n',
                    footer: '//@ sourceURL=turbo_small.js',
                    mangle: true
                },
                files: {
                    'dist/js/turbo_small.js': ['dist/js/turbo_small.js']
                }
            },
            turbo_small_lib: {
                options: {
                    banner: '/*! <%= pkg.name %> turbo_small_lib.js <%= grunt.template.today("dd-mm-yyyy") %> */\n',
                    footer: '//@ sourceURL=turbo_small_lib.js',
                    mangle: true
                },
                files: {
                    'dist/js/turbo_small_lib.js': ['dist/js/turbo_small_lib.js']
                }
            }
        }
    });

    grunt.loadNpmTasks('grunt-contrib-copy');
    grunt.loadNpmTasks('grunt-contrib-concat');
    grunt.loadNpmTasks('grunt-ngmin');
    grunt.loadNpmTasks('grunt-contrib-uglify');

    grunt.registerTask('concatAll', ['concat:app', 'concat:app_lib', 'concat:turbo', 'concat:turbo_lib', 'concat:turbo_small', 'concat:turbo_small_lib']);
    grunt.registerTask('ngminAll', ['ngmin:app', 'ngmin:turbo', 'ngmin:turbo_small']);
    grunt.registerTask('uglifyAll', ['uglify:app', 'uglify:app_lib', 'uglify:turbo', 'uglify:turbo_lib', 'uglify:turbo_small', 'uglify:turbo_small_lib']);

    grunt.registerTask('dist', ['copy', 'concatAll', 'ngminAll', 'uglifyAll']);
    grunt.registerTask('default', ['dist']);
};