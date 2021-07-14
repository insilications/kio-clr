add_library(kcm_proxy MODULE)

target_sources(kcm_proxy PRIVATE
    kproxydlg.cpp
    ../ksaveioconfig.cpp
)

ki18n_wrap_ui(kcm_proxy
    kproxydlg.ui
)

target_link_libraries(kcm_proxy PRIVATE
    Qt5::Widgets
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::I18n
    KF5::ConfigWidgets
    KF5::KIOWidgets
)

install(TARGETS kcm_proxy DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES proxy.desktop DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})