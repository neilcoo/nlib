project(nlib)

# TODO: add -pedantic
if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++17 -fPIC -DDEBUG -Wall -g3 -Og -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer -Wno-dangling-else -pthread)
else()
	add_compile_options(-std=gnu++17 -fPIC -Wall -O3 -fno-strict-aliasing -Wno-strict-aliasing -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer -Wno-dangling-else -pthread)
endif()


option(INCLUDE_POSTGRES "Include helpers for PostgreSQL" OFF)
option(INCLUDE_SQLITE   "Include helpers for SQLite"     OFF)
option(INCLUDE_LIBXML2  "Include helpers for libxml2"    OFF)
option(INCLUDE_LIBZMQ   "Include helpers for ZeroMQ"     OFF)

cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)
include_directories(..)

set(sourcefiles
    nargs.cxx
    nbinary.cxx
    nconfig.cxx
    ncrc.cxx
    nerror.cxx
    nevent.cxx
    nmutex.cxx
    nprocess.cxx
    nrandom.cxx
    nserial.cxx
    nsocketCan.cxx
    nsocket.cxx
    ntcpServer.cxx
    nthread.cxx
    nthreadPool.cxx
    ntime.cxx
    ntokeniser.cxx
)

if(INCLUDE_POSTGRES)
    list(APPEND sourcefiles npsql.cxx npsqlResult.cxx)
    add_definitions(-I/usr/include/postgresql)
endif(INCLUDE_POSTGRES)

if(INCLUDE_SQLITE)
    list(APPEND sourcefiles nsqlite.cxx)
    list(APPEND sourcefiles nsqliteResult.cxx)
endif(INCLUDE_SQLITE)

if(INCLUDE_LIBXML2)
    list(APPEND sourcefiles nxml.cxx)
    add_definitions(-I/usr/include/libxml2/)
endif(INCLUDE_LIBXML2)

if(INCLUDE_LIBZMQ)
    list(APPEND sourcefiles nzmq.cxx)
endif(INCLUDE_LIBZMQ)


add_library(nlib STATIC ${sourcefiles})
