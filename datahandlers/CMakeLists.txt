include_directories(
    ${kstars_SOURCE_DIR}/kstars/tools
    ${kstars_SOURCE_DIR}/kstars/skyobjects
    ${kstars_SOURCE_DIR}/kstars/skycomponents
    ${kstars_SOURCE_DIR}/kstars/auxiliary
    ${kstars_SOURCE_DIR}/kstars/time
)

set(LibKSDataHandlers_SRCS
        ${kstars_SOURCE_DIR}/datahandlers/catalogentrydata.cpp
        ${kstars_SOURCE_DIR}/datahandlers/catalogdata.cpp
        ${kstars_SOURCE_DIR}/datahandlers/ksparser.cpp
        ${kstars_SOURCE_DIR}/datahandlers/catalogdb.cpp
)

add_library(LibKSDataHandlers STATIC ${LibKSDataHandlers_SRCS})

# Added this because includedir was missing, is this required?
target_link_libraries(LibKSDataHandlers KF5::WidgetsAddons KF5::I18n Qt5::Sql Qt5::Core Qt5::Gui)

