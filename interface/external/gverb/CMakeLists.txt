cmake_minimum_required(VERSION 2.8)
add_library(gverb include/ladspa-util.h include/lv2.h src/gverb.h src/gverb.c src/gverbdsp.h src/gverbdsp.c)
