add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_shell\")

kcoreaddons_add_plugin(krunner_shell SOURCES shellrunner.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_shell
    KF6::KIOCore
    KF6::KIOGui
    KF6::I18n
    KF6::Notifications
    KF6::Plasma
    KF6::Runner
    KF6::Completion
)

if(BUILD_TESTING)
add_subdirectory(autotests)
endif()

