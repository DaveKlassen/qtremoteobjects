# Generated from repc.pro.

#####################################################################
## repc Tool:
#####################################################################

qt_get_tool_target_name(target_name repc)
qt_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Remote Objects Compiler"
    TOOLS_TARGET RemoteObjects # special case
    SOURCES
        cppcodegenerator.cpp cppcodegenerator.h
        main.cpp
        repcodegenerator.cpp repcodegenerator.h
        utils.cpp utils.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_CAST_TO_ASCII
        QT_NO_URL_CAST_FROM_STRING
        RO_INSTALL_HEADERS=\"$$[QT_INSTALL_HEADERS]/QtRemoteObjects\"
    INCLUDE_DIRECTORIES
        ${CMAKE_SOURCE_DIR}/src/repparser
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# QLALR Grammars:
qt_process_qlalr(
    ${target_name} # special case
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/repparser/parser.g" # special case
    ""
)

#### Keys ignored in scope 1:.:.:repc.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Qt Remote Objects Compiler"
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_extend_target(${target_name} CONDITION MSVC
    COMPILE_OPTIONS
        /wd4129
)
