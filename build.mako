
# this is a comment

# macros are expanded in place
macro cc { "gcc" }
macro cflags {
    "-Wall" "-Wextra" "-Werror" "-pedantic"
    "-L./strap" "-I./strap/src"
}
macro libs { "-lstrap" }

# ! is a separate token, but can be right in front of other tokens
"strap/libstrap.a" fileexists! if {
    cmd "/bin/sh" "strap/build.sh" run
}

cmd cc cflags "-o" "mako" "src/main.c" libs run
