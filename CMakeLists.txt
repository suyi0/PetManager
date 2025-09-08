cmake_minimum_required(VERSION 3.14)
project(PetManager)

find_package(OpenSSL REQUIRED)  # 查找 OpenSSL

# 使用 FetchContent 下载 nlohmann/json
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# 查找 MySQL Connector/C++ 9.x
find_package(PkgConfig REQUIRED)
# 使用 mysqlx 名称查找新版 MySQL Connector/C++
pkg_check_modules(MYSQLX REQUIRED mysqlx)

# 添加 MySQL Connector/C++ 的包含目录
include_directories(${MYSQLX_INCLUDE_DIRS})
link_directories(${MYSQLX_LIBRARY_DIRS})

include_directories(${OPENSSL_INCLUDE_DIR})  # 添加头文件路径

# 添加所有源文件
set(SOURCES
    pethospital/src/api/Encrypt.cpp
    pethospital/src/api/GetAddress.cpp
    pethospital/src/api/ManagerBack.cpp
    pethospital/src/api/RunServe.cpp
    pethospital/src/api/User.cpp
    pethospital/src/api/Utils.cpp
    pethospital/src/api/Verification.cpp
)

# 创建可执行文件，使用 RunServe 作为目标名称
add_executable(RunServe ${SOURCES})

target_link_libraries(ManagerBack PRIVATE 
    boost_system pthread 
    ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY}  # 链接 OpenSSL
    ${MYSQLCPPCONN_LIBRARIES}
)
target_link_libraries(ManagerBack PRIVATE nlohmann_json::nlohmann_json)