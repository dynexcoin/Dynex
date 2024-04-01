
set(UPNPC_BUILD_STATIC ON CACHE BOOL "Build static library")
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "Build test executables")
set(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "Build sample executables")

add_subdirectory(miniupnpc)
#add_subdirectory(gtest)

#set_property(TARGET upnpc-static gtest gtest_main PROPERTY FOLDER "external")
