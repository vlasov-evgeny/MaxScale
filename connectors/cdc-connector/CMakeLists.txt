# Shared version of the library
add_library(cdc_connector SHARED cdc_connector.cpp)
add_dependencies(cdc_connector jansson)
target_link_libraries(cdc_connector ${JANSSON_LIBRARIES} crypto)
set_target_properties(cdc_connector PROPERTIES VERSION "1.0.1")
add_dependencies(cdc_connector jansson)

# Static version of the library
add_library(cdc_connector_static STATIC cdc_connector.cpp)
add_dependencies(cdc_connector_static jansson)
set_target_properties(cdc_connector_static PROPERTIES OUTPUT_NAME cdc_connector)
add_dependencies(cdc_connector_static jansson)

install_dev_library(cdc_connector cdc-connector)
install_dev_library(cdc_connector_static cdc-connector)
install_header(cdc_connector.h cdc-connector)
install_file(LICENSE cdc-connector)

set(cdc-connector_PACKAGE_SUMMARY "MaxScale CDC Connector" CACHE INTERNAL "")
set(cdc-connector_PACKAGE_DESCRIPTION "The C++ connector for the MariaDB MaxScale CDC system." CACHE INTERNAL "")
set(cdc-connector_LICENSE "LGPLv2.1" CACHE INTERNAL "")
