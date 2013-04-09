
project(starfield)

# Only headers (that are facaded by the Stars.cpp file) here -
# hence declared as custom target.

file(GLOB_RECURSE STARFIELD_SRCS *.h)
add_custom_target("starfield" SOURCES ${STARFIELD_SRCS})

