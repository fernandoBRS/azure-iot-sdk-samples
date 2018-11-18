cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
set(AZUREIOT_INC_FOLDER ".." "/usr/include/azureiot" "/usr/include/azureiot/inc")
set(INCLUDE_FOLDER "./include")
set(SRC_FOLDER "./src")

set(CMAKE_BINARY_DIR "./bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${AZUREIOT_INC_FOLDER} ${INCLUDE_FOLDER} ${SRC_FOLDER})

set(application_c_files
    ${SRC_FOLDER}/main.c
    ${SRC_FOLDER}/mqtt_sample.c
)

set(application_h_files
    ${INCLUDE_FOLDER}/mqtt_sample.h
)

add_executable(app ${application_c_files} ${application_h_files})

target_link_libraries(
    app
    iothub_client
    iothub_client_mqtt_transport
    aziotsharedutil
    serializer
    prov_auth_client
    hsm_security_client
    uhttp
    parson
    uuid
    curl
    ssl
    umqtt
    pthread
    crypto
    m
)

















