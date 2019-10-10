cmake_minimum_required (VERSION 3.1)
project (sqlsmith)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(SOURCES relmodel.cc schema.cc 
	random.cc prod.cc expr.cc grammar.cc 
	log.cc dump.cc impedance.cc	
    sqlsmith.cc)

list(APPEND SOURCES mysql.cc postgres.cc)

find_package(MySQL REQUIRED)
include_directories(${MYSQL_INCLUDE_DIR})

find_package(POSTGRES REQUIRED)
include_directories(${POSTGRES_INCLUDE_DIRECTORIES})

find_package(PQXX REQUIRED)
include_directories(${PQXX_INCLUDE_DIRECTORIES})


find_package(SQLite3)
if(SQlite3_FOUND)
	include_directories(${SQLite3_INCLUDE_DIRS})
ENDIF()

add_executable(sqlsmith ${SOURCES})

# Use fixed definition 
set(PACKAGE "sqlsmith")
set(PACKAGE_BUGREPORT "eltenreich@gmx.de")
set(PACKAGE_NAME "SQLsmith")
set(PACKAGE_STRING "SQLsmith 1.2.1")
set(PACKAGE_TARNAME "sqlsmith")
set(PACKAGE_URL "https://github.com/anse1/sqlsmith/")
set(PACKAGE_VERSION "1.2.1")
set(VERSION "1.2.1")
configure_file(${CMAKE_SOURCE_DIR}/cmake/config.h.in ${CMAKE_SOURCE_DIR}/config.h)

execute_process(COMMAND git describe --dirty --tags --always OUTPUT_VARIABLE GITREV)
string(STRIP GITREV GITREV)

configure_file(${CMAKE_SOURCE_DIR}/cmake/gitrev.h.in ${CMAKE_SOURCE_DIR}/gitrev.h)

set(LIBS ${MYSQL_LIBRARIES})
list(APPEND LIBS ${POSTGRES_LIBRARIES} ${PQXX_LIBRARIES})

if(SQLite3_FOUND)
	list(APPEND LIBS ${SQLite3_LIBRARIES})
ENDIF()

target_link_libraries(sqlsmith ${LIBS})
