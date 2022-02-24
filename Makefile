# Project Name
TARGET = Terrarium

# Sources
SRC_DIR=src

CPP_SOURCES = src/Terrarium.cpp \
src/lib/theory.cpp

# Library Locations
LIBDAISY_DIR = ../DaisyExamples/libdaisy
DAISYSP_DIR = ../DaisyExamples/DaisySP

# Compiler flag to optimize for binary size. Reduces bin size by ~10%
OPT = -Os
# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

