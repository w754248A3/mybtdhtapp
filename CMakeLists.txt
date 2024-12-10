cmake_minimum_required(VERSION 3.16)

project(mydhtrecord CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MY_DIR_PATH ${PROJECT_SOURCE_DIR})

function(my_set_common_properties target)

    # set_target_properties(${target} PROPERTIES 
    #                         CMAKE_CXX_STANDARD 20
    #                         CMAKE_CXX_STANDARD_REQUIRED ON)
  
    target_compile_definitions(${target} PUBLIC
        _WIN32_WINNT=0x0602
        NTDDI_VERSION=0x06020000
    )
   
    target_compile_options(${target} PUBLIC
        -pedantic-errors
        -Woverflow
        -Wconversion
        -Werror
        -Wextra
        -fconcepts
    )

    target_link_options(${target} PUBLIC
    -static)
   
    target_include_directories(${target} PUBLIC
        ${MY_DIR_PATH}/include
        ${MY_DIR_PATH}/../CppHttpFileServer
    )


    


    target_link_libraries(${target} PUBLIC
        torrent-rasterbar
        boost_atomic-mt
        boost_charconv-mt
        boost_chrono-mt
        boost_container-mt
        boost_context-mt
        boost_contract-mt
        boost_coroutine-mt
        boost_date_time-mt
        boost_exception-mt
        boost_fiber-mt
        boost_filesystem-mt
        boost_graph-mt
        boost_iostreams-mt
        boost_json-mt
        boost_locale-mt
        boost_log-mt
        boost_log_setup-mt
        boost_math_c99-mt
        boost_math_c99f-mt
        boost_math_c99l-mt
        boost_math_tr1-mt
        boost_math_tr1f-mt
        boost_math_tr1l-mt
        boost_nowide-mt
        boost_numpy311-mt
        boost_prg_exec_monitor-mt
        boost_program_options-mt
        boost_python311-mt
        boost_random-mt
        boost_regex-mt
        boost_serialization-mt
        boost_stacktrace_noop-mt
        boost_stacktrace_windbg-mt
        boost_stacktrace_windbg_cached-mt
        boost_system-mt
        boost_test_exec_monitor-mt
        boost_thread-mt
        boost_timer-mt
        boost_type_erasure-mt
        boost_unit_test_framework-mt
        boost_url-mt
        boost_wave-mt
        boost_wserialization-mt
        sqlite3
        ssl
        crypto
        crypt32
        ws2_32
        wininet
        shlwapi
        Iphlpapi
        Mswsock
    )

   

endfunction()


function(my_set_target_out targetname)
    # 设置输出目录
    set_target_properties(${targetname} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${MY_DIR_PATH}/out
    OUTPUT_NAME "${targetname}"
    )
endfunction()


add_executable(mydhtrecord ./mydhtrecord.cpp)


my_set_common_properties(mydhtrecord)


target_precompile_headers(mydhtrecord PUBLIC
    <libtorrent/session.hpp>
    <libtorrent/add_torrent_params.hpp>
    <libtorrent/torrent_handle.hpp>
    <libtorrent/magnet_uri.hpp>
    <libtorrent/alert_types.hpp>
    <libtorrent/bdecode.hpp>
    <libtorrent/bencode.hpp>
    <libtorrent/create_torrent.hpp>
    )


my_set_target_out(mydhtrecord)




add_executable(testsqlite ./testsqlite.cpp)

my_set_common_properties(testsqlite)

target_precompile_headers(testsqlite REUSE_FROM mydhtrecord)

my_set_target_out(testsqlite)