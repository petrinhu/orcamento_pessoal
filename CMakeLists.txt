cmake_minimum_required(VERSION 3.16)

project(orcamento-pessoal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Sql)
find_package(OpenSSL REQUIRED)

qt_add_executable(orcamento-pessoal
    resources/resources.qrc
    src/main.cpp
    src/core/CryptoHelper.cpp
    src/core/DatabaseManager.cpp
    src/models/Categoria.cpp
    src/models/Entrada.cpp
    src/models/GastoFixo.cpp
    src/models/GastoVariavel.cpp
    src/ui/Theme.cpp
    src/ui/ConfigWidget.cpp
    src/ui/DashboardWidget.cpp
    src/ui/EntradasWidget.cpp
    src/ui/GastosFixosWidget.cpp
    src/ui/GastosVariaveisWidget.cpp
    src/ui/MainWindow.cpp
    src/ui/PasswordDialog.cpp
    src/utils/CurrencyUtils.cpp
)

target_include_directories(orcamento-pessoal PRIVATE src)

target_link_libraries(orcamento-pessoal PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    Qt6::Sql
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_compile_options(orcamento-pessoal PRIVATE -march=native -O2 -Wall)
