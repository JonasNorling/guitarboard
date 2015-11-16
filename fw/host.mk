#
# Makefile with definitions for building for host (POSIX)
#

BUILDDIR := build_host

COMMONFLAGS := -I. -Isrc -Isrc/host -Iinclude
COMMONFLAGS += -std=c11 -O1 -fno-common -g -Wall -Wextra -DHOST
COMMONFLAGS += -Wall -Wextra -Werror-implicit-function-declaration  -Werror -Wno-error=unused-variable
COMMONFLAGS += $(shell pkg-config --cflags jack)

LDFLAGS += $(shell pkg-config --libs jack) -lm

# Sources to build for host only
SRCS += src/host/platform-host.c
SRCS += src/host/jackclient.c

COMMON_OBJS := $(SRCS:src/%.c=$(BUILDDIR)/%.o)

include common.mk
