
include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
  find_package(Qt5Gui 5.15.8 EXACT REQUIRED PATHS "/home/linuxbrew/.linuxbrew/opt/qt@5/" NO_DEFAULT_PATH) # force x86 version on M1
  find_package(Qt5Widgets 5.15.8 EXACT REQUIRED PATHS "/home/linuxbrew/.linuxbrew/opt/qt@5/" NO_DEFAULT_PATH) # force x86 version on M1
  include_directories("/home/linuxbrew/.linuxbrew/opt/qt@5/include")
  link_directories("/home/linuxbrew/.linuxbrew/opt/qt@5/lib")
else()
  find_package(Qt5Gui 5.15 REQUIRED)
  find_package(Qt5Widgets 5.15 REQUIRED)
endif()

set(VERSION "")

set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE FORMS gui/ui/*.ui)

set(QRC resources.qrc)

qt5_wrap_ui(UIS ${FORMS})
qt5_add_resources(RCC ${QRC})


if (WIN32)
  if (NOT MSVC)
	#message(FATAL_ERROR "Only MSVC is supported on this platform")
  endif ()
  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0600 /DSTATICLIB)

  set(PLATFORM_DIR Windows)
  set(BUILD_PLATFORM WIN32)
  set(BUILD_RESOURCES cryptonotewallet.rc)
  set(QTMAIN Qt5::WinMain)

elseif (UNIX)
  set(PLATFORM_DIR Posix)
  #set(CRYPTONOTE_SOURCES ${CRYPTONOTE_SOURCES} cryptonote/external/miniupnpc/minissdpc.c)
  if (APPLE)
    enable_language(ASM)
    file(GLOB_RECURSE OBJC_SOURCES gui/*.mm)
    set(SOURCES ${SOURCES} ${OBJC_SOURCES})
    set(PLATFORM_DIR OSX)
    set(MACOSX_BUNDLE_INFO_STRING "Dynex GUI wallet")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_VERSION}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(MACOSX_BUNDLE_BUNDLE_NAME DynexWallet)
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_VERSION}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "$${VERSION_VERSION}.${VERSION_MINOR}.${VERSION_PATCH}")

    find_package(Qt5PrintSupport REQUIRED)

    include_directories(/usr/include/malloc)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes -std=c++11 -stdlib=libc++ -lboost_program_options -lboost_filesystem -L /home/linuxbrew/.linuxbrew/Cellar/boost/1.81.0_1/include -L /home/linuxbrew/.linuxbrew/Cellar/boost/1.81.0_1/lib")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -maes -D_DARWIN_C_SOURCE ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework CoreFoundation -framework Carbon -framework IOKit")

    set(MACOSX_BUNDLE_ICON_FILE cryptonote.icns)
    set(APPLICATION_ICON images/cryptonote.icns)
    set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(BUILD_PLATFORM MACOSX_BUNDLE)
    set(BUILD_RESOURCES ${APPLICATION_ICON})

    GET_TARGET_PROPERTY(QT_LIB_DIR "${Qt5Widgets_LIBRARIES}" LOCATION)
    GET_FILENAME_COMPONENT(QT_LIB_DIR "${QT_LIB_DIR}" PATH)
  else(APPLE)
    set(PLATFORM_DIR Linux)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes -std=c++11 -lboost_program_options -lboost_filesystem")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes -std=c11 ") #-L/home/linuxbrew/.linuxbrew/opt/qt@5/lib -I/home/linuxbrew/.linuxbrew/opt/qt@5/include
  endif (APPLE)
endif ()

add_executable(WalletGui ${BUILD_PLATFORM} ${BUILD_RESOURCES} ${SOURCES} ${HEADERS} ${UIS} ${RCC})
set_target_properties(WalletGui PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)
target_link_libraries(WalletGui Wallet Mnemonics NodeRpcProxy Transfers DynexCNCore Crypto P2P Rpc Http Serialization System Logging Common InProcessNode BlockchainExplorer libminiupnpc-static ${Boost_LIBRARIES} ${CURL_LIBRARIES})

if (APPLE)
  qt5_use_modules(WalletGui PrintSupport)
elseif (UNIX)
  target_link_libraries(WalletGui -lpthread)
elseif (WIN32)
  target_link_libraries(WalletGui Imm32 Iphlpapi Winmm)
endif (APPLE)

#qt5_use_modules(WalletGui Widgets Gui)
target_link_libraries(WalletGui Qt5::Gui Qt5::Widgets)

set_property(TARGET WalletGui PROPERTY OUTPUT_NAME "dynexwallet")
