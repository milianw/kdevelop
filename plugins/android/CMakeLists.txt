add_definitions(-DTRANSLATION_DOMAIN=\"kdevandroid\")

ecm_qt_declare_logging_category(androidplugin_SRCS
    HEADER debug_android.h
    IDENTIFIER ANDROID
    CATEGORY_NAME "kdevelop.plugins.android"
)
qt5_add_resources(androidplugin_SRCS kdevandroidplugin.qrc)
ki18n_wrap_ui(androidplugin_SRCS androidpreferences.ui)
kconfig_add_kcfg_files(androidplugin_SRCS androidpreferencessettings.kcfgc)

kdevplatform_add_plugin(kdevandroid JSON kdevandroid.json SOURCES androidplugin.cpp androidruntime.cpp androidpreferences.cpp ${androidplugin_SRCS})
target_link_libraries(kdevandroid
    KF5::CoreAddons
    KDev::Interfaces
    KDev::Util
    KDev::OutputView
    KDev::Project
)
