
if(DEFINED ENV{COPPELIA_REMOTE_API_DIR})
    set(COPPELIA_REMOTE_API_DIR "$ENV{COPPELIASIM_ROOT_DIR}/programming/zmqRemoteApi/clients/cpp")

    add_subdirectory(${COPPELIA_REMOTE_API_DIR} ${CMAKE_BINARY_DIR}/zmqRemoteApi)
    add_target_compile_definitions(gaden COPPELIA_REMOTE_API=1)
endif()