
include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt6 COMPONENTS Core Gui Widgets)
if (NOT Qt6_FOUND OR DEFINED QT5)
  find_package(Qt5 5.15 COMPONENTS Core Gui Widgets)
endif()

if (NOT Qt6_FOUND AND NOT Qt5_FOUND)
 MESSAGE(SEND_ERROR "QT6 or QT5 is required! (QT may depend on libglx-dev libgl1-mesa-dev packages)")
endif()


set(VERSION "")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE FORMS gui/ui/*.ui)

set(QRC resources.qrc)

qt_wrap_ui(UIS ${FORMS})
qt_add_resources(RCC ${QRC})

if (WIN32)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0600 /DSTATICLIB)

  set(PLATFORM_DIR Windows)
  set(BUILD_PLATFORM WIN32)
  set(BUILD_RESOURCES cryptonotewallet.rc)
  set(QTMAIN Qt::WinMain)

elseif (APPLE)
    set(PLATFORM_DIR OSX)

    file(GLOB_RECURSE OBJC_SOURCES gui/*.mm)
    set(SOURCES ${SOURCES} ${OBJC_SOURCES})

    set(MACOSX_BUNDLE_INFO_STRING "Dynex wallet ${CN_VER_MAJOR}.${CN_VER_MINOR}.${CN_VER_REV}.${CN_VER_WAL}")
    set(MACOSX_BUNDLE_BUNDLE_NAME DynexWallet)
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${CN_VER_MAJOR}.${CN_VER_MINOR}.${CN_VER_REV}.${CN_VER_WAL}-${CN_VER_BUILD}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${CN_VER_MAJOR}.${CN_VER_MINOR}.${CN_VER_REV}.${CN_VER_WAL}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${CN_VER_MAJOR}.${CN_VER_MINOR}.${CN_VER_REV}.${CN_VER_WAL}")

    set(MACOSX_BUNDLE_ICON_FILE dynex.icns)
    set(APPLICATION_ICON images/dynex.icns)
    set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(BUILD_PLATFORM MACOSX_BUNDLE)
    set(BUILD_RESOURCES ${APPLICATION_ICON})

    #include_directories(/usr/include/malloc)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes -std=c++17 -stdlib=libc++ -lboost_program_options -lboost_filesystem -L /home/linuxbrew/.linuxbrew/Cellar/boost/1.81.0_1/include -L /home/linuxbrew/.linuxbrew/Cellar/boost/1.81.0_1/lib")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -maes -D_DARWIN_C_SOURCE ")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework CoreFoundation -framework Carbon -framework IOKit")
elseif (UNIX)
  #set(PLATFORM_DIR Posix)
  set(PLATFORM_DIR Linux)
endif()

add_executable(WalletGui ${BUILD_PLATFORM} ${BUILD_RESOURCES} ${SOURCES} ${HEADERS} ${UIS} ${RCC})
set_target_properties(WalletGui PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)

target_link_libraries(WalletGui Wallet Mnemonics NodeRpcProxy Transfers DynexCNCore Crypto P2P Rpc Http Serialization System Logging Common InProcessNode BlockchainExplorer libminiupnpc-static ${Boost_LIBRARIES} ${CURL_LIBRARIES})

target_link_libraries(WalletGui Qt::Core Qt::Gui Qt::Widgets)
set_property(TARGET WalletGui PROPERTY OUTPUT_NAME "dynexwallet")

#if (UNIX)
#  target_link_libraries(WalletGui -lpthread)
#endif()
if (APPLE)
  set_target_properties(WalletGui PROPERTIES MACOSX_BUNDLE ON)
elseif (WIN32)
  target_link_libraries(WalletGui Imm32 Iphlpapi Winmm)
  set_target_properties(WalletGui PROPERTIES WIN32_EXECUTABLE ON)
endif()


if (Qt6_FOUND)
#set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/src/WalletGui)

#if(CMAKE_VERSION VERSION_LESS 3.13.0)
#  MESSAGE(WARNING "QT6 DEPLOYMENT NOT SUPPORTED WITH CMAKE < 3.14")
#endif()

if(POLICY CMP0087)
  cmake_policy(SET CMP0087 NEW)
endif()

if(APPLE AND DEFINED BUNDLE)
  #install(TARGETS WalletGui BUNDLE DESTINATION .)
  #qt_generate_deploy_app_script(TARGET WalletGui OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
  #install(SCRIPT ${deploy_script})

  target_link_libraries(WalletGui Qt::DBus) # it used internally by QtGui from homebrew
  get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
  get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
  find_program(DEPLOYQT macdeployqt HINTS "${_qt_bin_dir}")
  add_custom_command(TARGET WalletGui POST_BUILD COMMAND 
    "${DEPLOYQT}" ARGS "dynexwallet.app" 
    -codesign=-
    -hardened-runtime
    -dmg
    #-verbose=2
    COMMENT "Creating MacOS bundle")

elseif(WIN32)
  get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
  get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
  find_program(DEPLOYQT windeployqt6.exe PATHS "${_qt_bin_dir}")
  add_custom_command(TARGET WalletGui POST_BUILD COMMAND 
    "${DEPLOYQT}" ARGS "$<TARGET_FILE:WalletGui>" --pdb
    --compiler-runtime --no-translations --no-system-d3d-compiler 
    --skip-plugin-types generic,networkinformation --exclude-plugins qtiff,qwebp
    #--verbose=2 
  )
endif()

endif(Qt6_FOUND)
