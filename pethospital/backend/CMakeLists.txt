cmake_minimum_required(VERSION 3.16)
project(PetManager VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Crow定义
add_definitions(-DCROW_ENABLE_MULTIPART)

# 查找必需包
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

# 设置Homebrew路径
if(NOT DEFINED HOMEBREW_PREFIX)
    if(EXISTS "/opt/homebrew")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    elseif(EXISTS "/usr/local")
        set(HOMEBREW_PREFIX "/usr/local")
    else()
        set(HOMEBREW_PREFIX "")
    endif()
endif()

message(STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")

# 自动查找 Boost - 专门处理 Boost 1.90.0 的 header-only system 库
function(find_boost_automatic)
    message(STATUS "Searching for Boost libraries...")
    
    # 查找包含目录
    find_path(BOOST_INCLUDE_DIR
        NAMES boost/version.hpp
        PATHS ${HOMEBREW_PREFIX}/include
              ${HOMEBREW_PREFIX}/Cellar/boost/*/include
              /usr/local/include
        DOC "Path to Boost include directory"
    )
    
    if(NOT BOOST_INCLUDE_DIR)
        message(FATAL_ERROR "Could not find Boost include directory. Please install Boost: brew install boost")
    endif()
    
    # 读取Boost版本
    file(READ "${BOOST_INCLUDE_DIR}/boost/version.hpp" boost_version_content)
    string(REGEX MATCH "#define BOOST_VERSION ([0-9]+)" _ ${boost_version_content})
    if(CMAKE_MATCH_1)
        math(EXPR BOOST_MAJOR_VERSION "${CMAKE_MATCH_1} / 100000")
        math(EXPR BOOST_MINOR_VERSION "${CMAKE_MATCH_1} / 100 % 1000")
        math(EXPR BOOST_PATCH_VERSION "${CMAKE_MATCH_1} % 100")
        message(STATUS "Boost version: ${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_PATCH_VERSION}")
        
        # 检查Boost版本是否>=1.90.0（system库是header-only）
        if(BOOST_MAJOR_VERSION EQUAL 1 AND BOOST_MINOR_VERSION GREATER_EQUAL 90)
            message(STATUS "Boost system library is header-only (version >= 1.90.0)")
            set(BOOST_SYSTEM_HEADER_ONLY TRUE PARENT_SCOPE)
        endif()
    endif()
    
    # 查找filesystem库（这是必需的库文件）
    find_library(BOOST_FILESYSTEM_LIBRARY
        NAMES boost_filesystem libboost_filesystem boost_filesystem-mt libboost_filesystem-mt
        PATHS ${HOMEBREW_PREFIX}/lib
              ${HOMEBREW_PREFIX}/Cellar/boost/*/lib
              /usr/local/lib
        PATH_SUFFIXES "" lib
        DOC "Path to Boost filesystem library"
    )
    
    # 如果找不到filesystem库，尝试直接查找Cellar中的实际文件
    if(NOT BOOST_FILESYSTEM_LIBRARY)
        find_library(BOOST_FILESYSTEM_LIBRARY_DIRECT
            NAMES libboost_filesystem.dylib libboost_filesystem.a libboost_filesystem-mt.dylib libboost_filesystem-mt.a
            PATHS ${HOMEBREW_PREFIX}/Cellar/boost/*/lib
            NO_DEFAULT_PATH
            DOC "Direct path to Boost filesystem library"
        )
        if(BOOST_FILESYSTEM_LIBRARY_DIRECT)
            set(BOOST_FILESYSTEM_LIBRARY ${BOOST_FILESYSTEM_LIBRARY_DIRECT} CACHE STRING "Boost filesystem library" FORCE)
        endif()
    endif()
    
    # 查找可选的thread库
    find_library(BOOST_THREAD_LIBRARY
        NAMES boost_thread libboost_thread boost_thread-mt libboost_thread-mt
        PATHS ${HOMEBREW_PREFIX}/lib
              ${HOMEBREW_PREFIX}/Cellar/boost/*/lib
              /usr/local/lib
        PATH_SUFFIXES "" lib
        DOC "Path to Boost thread library"
    )
    
    if(NOT BOOST_THREAD_LIBRARY)
        find_library(BOOST_THREAD_LIBRARY_DIRECT
            NAMES libboost_thread.dylib libboost_thread.a libboost_thread-mt.dylib libboost_thread-mt.a
            PATHS ${HOMEBREW_PREFIX}/Cellar/boost/*/lib
            NO_DEFAULT_PATH
            DOC "Direct path to Boost thread library"
        )
        if(BOOST_THREAD_LIBRARY_DIRECT)
            set(BOOST_THREAD_LIBRARY ${BOOST_THREAD_LIBRARY_DIRECT} CACHE STRING "Boost thread library" FORCE)
        endif()
    endif()
    
    # 检查结果
    if(BOOST_INCLUDE_DIR AND BOOST_FILESYSTEM_LIBRARY)
        set(BOOST_FOUND TRUE PARENT_SCOPE)
        set(BOOST_INCLUDE_DIRS ${BOOST_INCLUDE_DIR} PARENT_SCOPE)
        
        # 构建库列表
        if(BOOST_THREAD_LIBRARY)
            set(BOOST_LIBRARIES ${BOOST_FILESYSTEM_LIBRARY} ${BOOST_THREAD_LIBRARY} PARENT_SCOPE)
        else()
            set(BOOST_LIBRARIES ${BOOST_FILESYSTEM_LIBRARY} PARENT_SCOPE)
        endif()
        
        message(STATUS "Found Boost successfully:")
        message(STATUS "  Include: ${BOOST_INCLUDE_DIR}")
        message(STATUS "  Filesystem library: ${BOOST_FILESYSTEM_LIBRARY}")
        if(BOOST_THREAD_LIBRARY)
            message(STATUS "  Thread library: ${BOOST_THREAD_LIBRARY}")
        endif()
        if(BOOST_SYSTEM_HEADER_ONLY)
            message(STATUS "  System library: header-only")
        endif()
    else()
        message(STATUS "Search results:")
        message(STATUS "  BOOST_INCLUDE_DIR: ${BOOST_INCLUDE_DIR}")
        message(STATUS "  BOOST_FILESYSTEM_LIBRARY: ${BOOST_FILESYSTEM_LIBRARY}")
        message(FATAL_ERROR "Could not find required Boost filesystem library. Please ensure Boost is installed: brew install boost")
    endif()
endfunction()

# 执行自动查找
find_boost_automatic()

# 统一的库查找函数
function(find_library_automatic LIB_NAME HEADER_NAME PKG_CONFIG_NAME)
    # 方式1: 通过 pkg-config
    if(PKG_CONFIG_FOUND AND PKG_CONFIG_NAME)
        pkg_check_modules(${LIB_NAME}_PC QUIET ${PKG_CONFIG_NAME})
        if(${LIB_NAME}_PC_FOUND)
            set(${LIB_NAME}_FOUND TRUE PARENT_SCOPE)
            set(${LIB_NAME}_INCLUDE_DIRS ${${LIB_NAME}_PC_INCLUDE_DIRS} PARENT_SCOPE)
            set(${LIB_NAME}_LIBRARIES ${${LIB_NAME}_PC_LIBRARIES} PARENT_SCOPE)
            message(STATUS "Found ${LIB_NAME} via pkg-config")
            return()
        endif()
    endif()

    # 方式2: 直接查找头文件和库
    find_path(${LIB_NAME}_INCLUDE_DIR 
        NAMES ${HEADER_NAME}
        PATHS ${CMAKE_PREFIX_PATH}
              ${CMAKE_INSTALL_PREFIX}/include
              /usr/local/include
              /opt/local/include
              /opt/homebrew/include
              /opt/homebrew/Cellar/${LIB_NAME}/*/include
              $ENV{HOME}/.local/include
        DOC "Path to ${LIB_NAME} header files"
    )

    # 查找库文件（如果需要）
    if(ARGN)  # 如果提供了额外的参数（库名）
        find_library(${LIB_NAME}_LIBRARY 
            NAMES ${ARGN}
            PATHS /usr/local/lib
                  /opt/local/lib
                  /opt/homebrew/lib
                  /opt/homebrew/Cellar/${LIB_NAME}/*/lib
            DOC "Path to ${LIB_NAME} library"
        )
        
        if(${LIB_NAME}_INCLUDE_DIR AND ${LIB_NAME}_LIBRARY)
            set(${LIB_NAME}_FOUND TRUE PARENT_SCOPE)
            set(${LIB_NAME}_INCLUDE_DIRS ${${LIB_NAME}_INCLUDE_DIR} PARENT_SCOPE)
            set(${LIB_NAME}_LIBRARIES ${${LIB_NAME}_LIBRARY} PARENT_SCOPE)
            message(STATUS "Found ${LIB_NAME}: ${${LIB_NAME}_INCLUDE_DIR}")
        endif()
    else()
        # 只需要头文件的库（如 Crow, nlohmann/json）
        if(${LIB_NAME}_INCLUDE_DIR)
            set(${LIB_NAME}_FOUND TRUE PARENT_SCOPE)
            set(${LIB_NAME}_INCLUDE_DIRS ${${LIB_NAME}_INCLUDE_DIR} PARENT_SCOPE)
            message(STATUS "Found ${LIB_NAME}: ${${LIB_NAME}_INCLUDE_DIR}")
        endif()
    endif()
endfunction()

# 查找 MySQL Connector/C++
find_library_automatic(MYSQLX "mysqlx/xdevapi.h" "mysqlx" "mysqlcppconnx")

if(NOT MYSQLX_FOUND)
    message(FATAL_ERROR "Could not find MySQL Connector/C++. Please install it with: brew install mysql-connector-c++")
endif()

# 查找 Crow（只需要头文件）
find_library_automatic(CROW "crow.h" "")

if(NOT CROW_FOUND)
    message(WARNING "Crow not found automatically")
    message(STATUS "Try: brew install crow")
endif()

# 查找 hiredis（Redis 内存中转站客户端）
# 传空 pkg-config 名以走 find_library 全路径解析（与 MYSQLX 一致），
# 避免 pkg-config 分支只给库名、不给 -L 目录导致链接期找不到库。
find_library_automatic(HIREDIS "hiredis/hiredis.h" "" "hiredis")

if(NOT HIREDIS_FOUND)
    # RedisClient.cpp 始终参与编译且硬用 hiredis，缺它必然编译/链接失败；
    # 因此把 hiredis 作为硬依赖，在配置期就明确失败并给指引，而不是拖到链接期报晦涩错误。
    message(FATAL_ERROR "hiredis not found. It is a required dependency. Try: brew install hiredis")
endif()

# 处理 nlohmann/json - 优先使用本地文件，否则尝试下载
find_path(JSON_INCLUDE_DIR 
    NAMES nlohmann/json.hpp
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/include
          /opt/homebrew/include
          /usr/local/include
    NO_DEFAULT_PATH
)

if(JSON_INCLUDE_DIR)
    message(STATUS "Found nlohmann/json locally: ${JSON_INCLUDE_DIR}")
    include_directories(${JSON_INCLUDE_DIR})
else()
    message(STATUS "nlohmann/json not found locally, attempting to download...")
    # 使用 FetchContent 下载 nlohmann/json
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    # 设置策略版本以避免兼容性问题
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# 添加包含目录
include_directories(${MYSQLX_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
if(CROW_FOUND)
    include_directories(${CROW_INCLUDE_DIRS})
endif()
if(HIREDIS_FOUND)
    include_directories(${HIREDIS_INCLUDE_DIRS})
endif()

# 添加必要的包含目录（现代项目结构）
include_directories(database)
include_directories(models)
include_directories(modules)
include_directories(routes)
include_directories(controllers)
include_directories(utils)
include_directories(middleware)
include_directories(services)
include_directories(websocket)

# 收集所有源文件（自动递归查找）
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.cc"
)

# 排除build目录中的文件
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/tests/.*")

# 创建可执行文件
add_executable(main ${SOURCES})

# 查找 MySQL Connector/C++
find_library_automatic(MYSQLX "mysqlx/xdevapi.h" "" "mysqlcppconnx")

# 如果上面的方法不行，尝试这个
if(NOT MYSQLX_LIBRARIES)
    find_library(MYSQLX_LIBRARIES
        NAMES mysqlcppconnx
        PATHS ${CMAKE_PREFIX_PATH}/lib
              /usr/local/lib
              /opt/local/lib
              /opt/homebrew/lib
              /opt/homebrew/Cellar/mysql-connector-c++/*/lib
        DOC "Path to MySQL Connector/C++ X DevAPI library"
    )
    if(MYSQLX_LIBRARIES)
        set(MYSQLX_FOUND TRUE)
        get_filename_component(MYSQLX_LIBRARY_DIR ${MYSQLX_LIBRARIES} DIRECTORY)
        message(STATUS "Found MySQL Connector/C++ X DevAPI: ${MYSQLX_LIBRARIES}")
    endif()
endif()

if(NOT MYSQLX_FOUND)
    message(FATAL_ERROR "Could not find MySQL Connector/C++. Please install it with: brew install mysql-connector-c++")
endif()

# 在 CMakeLists.txt 文件中添加 curl 库查找
find_library(CURL_LIBRARY
    NAMES curl
    PATHS /usr/local/lib
          /opt/local/lib
          /opt/homebrew/lib
    DOC "Path to curl library"
)

if(CURL_LIBRARY)
    message(STATUS "Found curl library: ${CURL_LIBRARY}")
    set(EXTRA_LIBS ${CURL_LIBRARY})
else()
    message(WARNING "Could not find curl library")
endif()

# 链接库
target_link_libraries(main
    ${Boost_LIBRARIES}
    pthread
    ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY} # 添加 OpenSSL 库
    ${MYSQLX_LIBRARIES}
    ${EXTRA_LIBS}
)

# 链接 hiredis（Redis 客户端）
if(HIREDIS_FOUND)
    target_link_libraries(main ${HIREDIS_LIBRARIES})
endif()

# 只有当通过 FetchContent 下载时才链接 nlohmann_json
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(main nlohmann_json::nlohmann_json)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(static_file_handler_tests
        tests/static_file_handler_tests.cpp
        utils/staticFileHandler.cpp
    )
    add_test(NAME static_file_handler_tests COMMAND static_file_handler_tests)

    add_executable(rate_limit_middleware_tests
        tests/rate_limit_middleware_tests.cpp
        middleware/rateLimitMiddleware/rateLimitMiddleware.cpp
        services/redis/RedisClient.cpp
    )
    if(HIREDIS_FOUND)
        target_link_libraries(rate_limit_middleware_tests ${HIREDIS_LIBRARIES})
    endif()
    add_test(NAME rate_limit_middleware_tests COMMAND rate_limit_middleware_tests)

    add_executable(sms_script_runner_tests
        tests/sms_script_runner_tests.cpp
        controllers/auth/smsScriptRunner.cpp
    )
    add_test(NAME sms_script_runner_tests COMMAND sms_script_runner_tests)

    add_executable(auth_session_store_tests
        tests/auth_session_store_tests.cpp
        controllers/auth/jwtUtils/jwtUtils.cpp
        services/auth/AuthSessionStore.cpp
        services/redis/RedisClient.cpp
        services/redis/userRoleCache/UserRoleCache.cpp
        utils/Utils.cpp
        utils/dataScope/DataScope.cpp
        utils/permissions/Permissions.cpp
        utils/roleTypeUtils/roleTypeUtils.cpp
        utils/visibilityFilter/VisibilityFilter.cpp
    )
    target_link_libraries(auth_session_store_tests
        ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY}
        ${MYSQLX_LIBRARIES}
    )
    if(HIREDIS_FOUND)
        target_link_libraries(auth_session_store_tests ${HIREDIS_LIBRARIES})
    endif()
    add_test(NAME auth_session_store_tests COMMAND auth_session_store_tests)

    add_executable(auth_middleware_boundary_tests
        tests/auth_middleware_boundary_tests.cpp
    )
    target_compile_definitions(auth_middleware_boundary_tests PRIVATE
        AUTH_MIDDLEWARE_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/middleware/authMiddleware/authMiddleware.cpp"
        JWT_UTILS_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/controllers/auth/jwtUtils/jwtUtils.cpp"
        DOCTOR_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/doctorRoutes/doctorRoutes.cpp"
        ADMIN_HANDLER_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/controllers/modules/admin/adminHandler.cpp"
    )
    add_test(NAME auth_middleware_boundary_tests COMMAND auth_middleware_boundary_tests)

    add_executable(doctor_medicine_cache_tests
        tests/doctor_medicine_cache_tests.cpp
        services/redis/medicineRedisCache/MedicineRedisCache.cpp
        services/redis/RedisClient.cpp
    )
    if(HIREDIS_FOUND)
        target_link_libraries(doctor_medicine_cache_tests ${HIREDIS_LIBRARIES})
    endif()
    add_test(NAME doctor_medicine_cache_tests COMMAND doctor_medicine_cache_tests)

    add_executable(doctor_list_cache_tests
        tests/doctor_list_cache_tests.cpp
        services/redis/doctorListCache/DoctorListCache.cpp
        services/redis/redisJsonCache/RedisJsonCache.cpp
        services/redis/RedisClient.cpp
    )
    if(HIREDIS_FOUND)
        target_link_libraries(doctor_list_cache_tests ${HIREDIS_LIBRARIES})
    endif()
    add_test(NAME doctor_list_cache_tests COMMAND doctor_list_cache_tests)

    add_executable(user_role_cache_tests
        tests/user_role_cache_tests.cpp
        services/redis/userRoleCache/UserRoleCache.cpp
        services/redis/RedisClient.cpp
    )
    if(HIREDIS_FOUND)
        target_link_libraries(user_role_cache_tests ${HIREDIS_LIBRARIES})
    endif()
    add_test(NAME user_role_cache_tests COMMAND user_role_cache_tests)

    add_executable(role_permission_tests
        tests/role_permission_tests.cpp
        utils/permissions/Permissions.cpp
        utils/roleTypeUtils/roleTypeUtils.cpp
        services/redis/userRoleCache/UserRoleCache.cpp
        services/redis/RedisClient.cpp
    )
    target_link_libraries(role_permission_tests
        ${MYSQLX_LIBRARIES}
    )
    if(HIREDIS_FOUND)
        target_link_libraries(role_permission_tests ${HIREDIS_LIBRARIES})
    endif()
    add_test(NAME role_permission_tests COMMAND role_permission_tests)

    add_executable(permission_model_tests
        tests/permission_model_tests.cpp
        utils/permissions/Permissions.cpp
    )
    add_test(NAME permission_model_tests COMMAND permission_model_tests)

    add_executable(data_scope_tests
        tests/data_scope_tests.cpp
        utils/dataScope/DataScope.cpp
        utils/permissions/Permissions.cpp
        utils/visibilityFilter/VisibilityFilter.cpp
    )
    add_test(NAME data_scope_tests COMMAND data_scope_tests)

    add_executable(sensitive_permission_route_tests
        tests/sensitive_permission_route_tests.cpp
    )
    target_compile_definitions(sensitive_permission_route_tests PRIVATE
        ADMIN_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/adminRoutes/adminRoutes.cpp"
        FINANCE_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/financeRoutes/financeRoutes.cpp"
        BOSS_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/bossRoutes/bossRoutes.cpp"
        WAREHOUSE_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/warehouseManagerRoutes/warehouseManagerRoutes.cpp"
        PERSONNEL_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/personnelRoutes/personnelRoutes.cpp"
        DOCTOR_ROUTES_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/routes/doctorRoutes/doctorRoutes.cpp"
        OPERATION_LOGGER_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/services/logger/operationLogger.cpp"
    )
    add_test(NAME sensitive_permission_route_tests COMMAND sensitive_permission_route_tests)

    add_executable(visibility_filter_tests
        tests/visibility_filter_tests.cpp
        utils/visibilityFilter/VisibilityFilter.cpp
    )
    add_test(NAME visibility_filter_tests COMMAND visibility_filter_tests)
endif()

# 如果是header-only的system库，添加编译定义
if(BOOST_SYSTEM_HEADER_ONLY)
    target_compile_definitions(main PRIVATE BOOST_SYSTEM_HEADER_ONLY=1)
    message(STATUS "Added BOOST_SYSTEM_HEADER_ONLY definition for Boost ${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}")
endif()

# 添加编译后信息
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build completed: $<TARGET_FILE:main>"
)

message(STATUS "")
message(STATUS "===============================================================================")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: PetManager")
message(STATUS "  C++ Standard: 17")
message(STATUS "  Boost Include: ${BOOST_INCLUDE_DIR}")
message(STATUS "  Boost Libraries: ${BOOST_LIBRARIES}")
message(STATUS "  MySQL Connector: ${MYSQLX_LIBRARIES}")
message(STATUS "  System Library: ${BOOST_SYSTEM_HEADER_ONLY}")
message(STATUS "===============================================================================")
