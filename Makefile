RACK_DIR ?= ../..

FLAGS += -Idep/include
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/*.c)
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

RACK_DIR = /Applications/VCV\ Rack\ 2\ Free.app/Contents/Rack-SDK
include $(RACK_DIR)/plugin.mk
