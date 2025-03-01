# mako
Simple stack-based build recipe language. Inspired by
[porth](https://gitlab.com/tsoding/porth) and
[nob](https://github.com/tsoding/nob.h), both by Tsoding.

## Quickstart
```console
$ ./build.sh
$ ./mako
$ ./mako
...
```

_(`build.mako` in the root of the project is a build recipe, written in mako,
equivalent to `build.sh`)_

## Documentation
Defualt build script filename is `build.mako`. Using CMD arguments other name
may be specified. You can find an example build recipe for this exact software
in the root of the repository.

### Syntax
Comments start with `#`. The language is stack-based, but uses a standart
lexer, often used as a lexer for full-featured languages, so `fileexists!`
amounts to two tokens: `fileexists` and `!`.

### Command execution
Commands are executed with `run` keyword. Command is a chunk of the stack, and
the start of it is marked with `cmd` keyword. If it is not present, the whole
stack is used. E.g.:
```
cmd "gcc" "-o" "mako" "main.c" run
```

### Ifs
Mako has basic branching: ifs. `if` keyword takes a boolean of the top of the
stack, and skips a block if it is `false`. Else block may be specified, but it
isn't necessary. E.g.:
```
"somelib/libsome.a" fileexists! if {
    cmd "/bin/sh" "somelib/build.sh" run
} else {
    "some lib is already built, skipping" log
}
```

### While loops
`while` keyword expects a condition after it, and if it is true it executes the
code block after it, then loops back to the condition. E.g.:
```
"src/" listdir  # example stack: ("main.c" "file1.c" "file1.h" 3) <- top
while dup 0 > {
    1 - swap
    log
} drop
```

### Macros
Mako has a macro system: they are defined with `macro` keyword, followed by
macro name and macro body itself, in curly braces, e.g.:
```
macro name { "some string" }
```
Macros are expended in place, using just its name. A macro can be defined
inside another macro, but it is not really useful, as you will probably just
get `macro redefenition` error. A macro can also be defined inside an if block,
e.g.:
```
iswin if {
    macro cc { "x86_64-w64-mingw-gcc" }
} else {
    macro cc { "gcc" }
}
```

### Intrinsics
Mako has quite a lot of intrinsic commands and operations:
- `!`: Invert topmost boolean on the stack. `(a -- !a)`
- `debug`: Crash the program and print the stack. `( -- )`
- `dup`: Duplicate topmost item on the stack. `(a -- a a)`
- `swap`: Swap two topmost items on the stack. `(a b -- b a)`
- `over`: Duplicate second topmost item on the stack. `(a b -- a b a)`
- `rot`: Rotate three topmost items on the stack. `(a b c -- c a b)`
- `fileexists`: Takes a string from the stack and returns a boolean specifing
  if such file exists. `(a -- b)`
- `direxists`: Takes a string from the stack and returns a boolean specifing
  if such directory exists. `(a -- b)`
- `mkdir`: Takes a string from the stack and creates a directory with such
  name. `(a -- )`
- `cd`: Takes a string from the stack and changes CWD to it if it exists.
  `(a -- )`
- `getcwd`: Returns a string, holding CWD. `( -- a)`
- `listdir`: Returns a list with directory contents. `(a -- b c d ... n )`
- `fnmatch`: Returns a list with matched files. `(a -- b c d ... n )`
- `log`: Prints a string with following `INFO: ` and leading newline at the
  end. `(a -- )`
- `error`: Prints a string with following `ERROR: ` and leading newline at the
  end, and exits with non-zero exitcode. `(a -- )`
- `print`: Prints a string as-is. `(a -- )`

These all are invoked the same way macros are expanded: by just using its
name. Many of them will log to the STDOUT, so you don't need to. :^)

Also, the language has a few math operations:
- `+`, `-`, `*`, `/`: Binary math operations (a b -- a+/-/*//b)
- `>=`, `<=`, `>`, `<`, `=`: Integer comparisons (a b -- a=/>/</>=/<=b)