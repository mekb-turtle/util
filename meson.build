project(
    'util',
    'c',
    version: '0.1',
    license: 'MPL-2.0',
    default_options: ['warning_level=3'],
)

# define test files, these get compiled separately
tests = [
    {'files': files('test' / 'ini_parse.c')},
    {'files': files('test' / 'recurse.c')},
]

# define project metadata
url = 'https://github.com/mekb-turtle/util'
name = meson.project_name()
version = meson.project_version()

dependencies = []
dependencies_test = []

# used if you want to install a library
libdir = get_option('prefix') / get_option('libdir')

# begin meson configuration

add_project_arguments(
    f'-DPROJECT_NAME="@name@"',
    f'-DPROJECT_VERSION="@version@"',
    f'-DPROJECT_URL="@url@"',
    language: 'c',
)

fs = import('fs')

# loop tests
foreach test_ : tests
    test_name = fs.stem(test_['files'])  # basename without extension
    if test_.has_key('name')
        test_name = test_['name']        # custom test name
    endif

    extra_src = []
    if test_.has_key('src')
        extra_src = test_['src']
    endif

    test_exe = executable(
        test_name,
        sources: test_['files'] + extra_src,
        install: false,
        dependencies: dependencies_test,
    )

    args = {}
    foreach key, val : test_
        # skip files and name
        if key != 'files' and key != 'name' and key != 'src'
            args += {key: val}
        endif
    endforeach

    test(test_name, test_exe, kwargs: args)
endforeach
