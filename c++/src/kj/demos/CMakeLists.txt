add_custom_target(demos)

macro(kj_add_demo name)
    set(args ${ARGN})

    cmake_parse_arguments(
            parsed_args
            ""
            ""
            "SOURCES"
            ${args})

    set(target demo_${name})
    add_executable(${target} ${parsed_args_SOURCES})

    target_include_directories(${target}
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(${target} kj-http)
    if(WITH_OPENSSL)
        target_link_libraries(${target} kj-tls)
    endif()
    if(WITH_ZLIB)
        target_link_libraries(${target} kj-gzip)
    endif()

    set_target_properties(${target}
            PROPERTIES
            OUTPUT_NAME ${name}-demo)

    add_dependencies(demos ${target})
endmacro()

kj_add_demo(balus SOURCES balus-practice-main.cc balus-practice.cc)
kj_add_demo(balus-common SOURCES balus-common.cc)
kj_add_demo(echo-http-server SOURCES echo-http-server.cc)
kj_add_demo(simple-http-client SOURCES simple-http-client.cc)