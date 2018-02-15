AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = edge_demo

edge_demo_CFLAGS = -std=gnu99 -Wall -Werror -Wl,--no-undefined
edge_demo_LDADD = -L. -laf_edge -lpthread -levent -laf_ipc -laf_util

edge_demo_SOURCES = \
    demo/demo.c                             \
    $(NULL)
