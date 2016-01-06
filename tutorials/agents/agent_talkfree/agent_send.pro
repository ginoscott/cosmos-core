# COSMOS agent post example
# Tested on windows with MinGW and MSVC

# define the COSMOS source folder
COSMOS_SOURCE_CORE = $$PWD/../../..

TEMPLATE = app
CONFIG += console
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += c++11

MODULES += agentlib
include( $$COSMOS_SOURCE_CORE/cosmos-core.pri )

SOURCES += agent_send.cpp
