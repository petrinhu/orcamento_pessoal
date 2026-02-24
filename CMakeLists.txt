cmake_minimum_required(VERSION 3.16)

project(orcamento-pessoal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

qt_add_executable(orcamento-pessoal
    main.cpp
)

target_link_libraries(orcamento-pessoal PRIVATE Qt6::Core Qt6::Widgets Qt6::Charts)

# Otimizações para seu hardware
target_compile_options(orcamento-pessoal PRIVATE -march=native -O2)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Adiciona OpenSSL
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL encontrado: ${OPENSSL_VERSION}")
else()
    message(FATAL_ERROR "OpenSSL não encontrado! Instale com: sudo dnf install openssl-devel")
endif()

# ... seu qt_add_executable ...

target_link_libraries(orcamento-pessoal PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Otimizações (já tem -march=native)
target_compile_options(orcamento-pessoal PRIVATE -march=native -O2 -Wall)
