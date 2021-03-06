add_definitions(-DTRANSLATION_DOMAIN=\"kdevexternalscript\")
########### next target ###############

set(kdevexternalscript_PART_SRCS
    externalscriptplugin.cpp
    externalscriptview.cpp
    externalscriptitem.cpp
    externalscriptjob.cpp
    editexternalscript.cpp
)

ecm_qt_declare_logging_category(kdevexternalscript_PART_SRCS
    HEADER debug.h
    IDENTIFIER PLUGIN_EXTERNALSCRIPT
    CATEGORY_NAME "kdevelop.plugins.externalscript"
)

set(kdevexternalscript_PART_UI
    externalscriptview.ui
    editexternalscript.ui
)

ki18n_wrap_ui(kdevexternalscript_PART_SRCS ${kdevexternalscript_PART_UI})

qt5_add_resources(kdevexternalscript_PART_SRCS kdevexternalscript.qrc)
kdevplatform_add_plugin(kdevexternalscript JSON kdevexternalscript.json SOURCES ${kdevexternalscript_PART_SRCS})

target_link_libraries(kdevexternalscript
    KF5::TextEditor KF5::KIOWidgets KF5::Parts KF5::NewStuff
    KDev::Language KDev::Interfaces KDev::Project
    KDev::Util KDev::OutputView
)
