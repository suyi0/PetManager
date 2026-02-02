cmake_minimum_required(VERSION 3.14)
project(PetManager)

# 添加定义以启用Crow的multipart支持
add_definitions(-DCROW_ENABLE_MULTIPART)

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

# 添加必要的包含目录
include_directories(pethospital/src/backend/api/include)
include_directories(pethospital/src/backend/api/include/UserSide)
include_directories(pethospital/src/backend/api/include/DoctorSde)
include_directories(pethospital/src/backend/api/include/AdminSide)
include_directories(pethospital/src/backend/api/include/Shared)

# 定义各个模块的源文件
set(CORE_SOURCES
    pethospital/src/backend/api/src/RunServe.cpp
)

set(ADMIN_SOURCES
)

set(USER_SOURCES
    pethospital/src/backend/api/src/UserBack/UserRoutes/UserRoutes.cpp
    pethospital/src/backend/api/src/UserBack/UserRoutes/GetAddress/GetAddress.cpp
)

set(DOCTOR_SOURCES
)

set(DATABASE_SOURCES
    pethospital/src/backend/api/src/DatabaseManager/DatabaseManager.cpp
)

set(SHARED_SOURCES
    pethospital/src/backend/api/src/Shared/Utils/Utils.cpp
)

set(MANAGER_SOURCES
    pethospital/src/backend/api/src/ManagerBack/ManagerBack.cpp
)

# 添加所有源文件
set(SOURCES
    ${CORE_SOURCES}
    ${ADMIN_SOURCES}
    ${USER_SOURCES}
    ${DOCTOR_SOURCES}
    ${DATABASE_SOURCES}
    ${SHARED_SOURCES}
    ${MANAGER_SOURCES}
)

# 创建可执行文件，使用 Run 作为目标名称
add_executable(Run ${SOURCES})

target_link_libraries(Run
    boost_system pthread 
    ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY}  # 链接 OpenSSL
    ${MYSQLX_LIBRARIES}
)
target_link_libraries(Run nlohmann_json::nlohmann_json)