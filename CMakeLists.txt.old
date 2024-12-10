cmake_minimum_required(VERSION 3.16)

project(mybtdhtapp CXX)

# 设置C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#set(CMAKE_EXE_LINKER_FLAGS "-static")

add_compile_definitions(_WIN32_WINNT=0x0602) 
add_compile_definitions(NTDDI_VERSION=0x06020000) 

# 添加编译选项
add_compile_options(
    -pedantic-errors
    -Woverflow
    -Wconversion
    -Werror
    -Wextra
    -fconcepts
)

add_link_options(
    -static
)

# 设置包含路径
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../CppHttpFileServer
)


# 查找Boost库路径
#set(BOOST_ROOT "C:/Users/PC/AppData/Local/Programs/msys2/ucrt64")
#find_package(Boost REQUIRED)

# 添加源文件
add_executable(${PROJECT_NAME} mydhtrecord.cpp)


target_precompile_headers(${PROJECT_NAME}
    PRIVATE 
    <libtorrent/session.hpp>
    <libtorrent/add_torrent_params.hpp>
    <libtorrent/torrent_handle.hpp>
    <libtorrent/magnet_uri.hpp>
    <libtorrent/alert_types.hpp>
    <libtorrent/bdecode.hpp>
    <libtorrent/bencode.hpp>
    <libtorrent/create_torrent.hpp>

)

# 链接选项（来自任务配置）
target_link_libraries(${PROJECT_NAME}
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

add_subdirectory("mycmake")

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out
    OUTPUT_NAME "${PROJECT_NAME}"
)
