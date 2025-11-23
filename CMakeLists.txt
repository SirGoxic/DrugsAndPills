cmake_minimum_required(VERSION 3.30)
project(DrugsAndPills LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_SUPPRESS_REGENERATION True)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_VS_USE_DEBUG_LIBRARIES $<CONFIG:Debug>)

set(SOURCES 
	src/files.h
	src/fonts.h
	src/image_library.cpp
	src/image_library.h
	src/main.cpp
	src/messages.h
	src/png_reader.cpp
	src/png_reader.h
	src/record.cpp
	src/record.h
	src/record_checker.cpp
	src/record_checker.h
	src/serializer.cpp
	src/serializer.h
	src/settings.cpp
	src/settings.h
	src/time_utils.cpp
	src/time_utils.h
	src/ui_consts.h
	src/wnd/main_wnd.cpp
	src/wnd/main_wnd.h
	src/wnd/notification_wnd.cpp
	src/wnd/notification_wnd.h
	src/wnd/panel_wnd.cpp
	src/wnd/panel_wnd.h
	src/wnd/record_list_wnd.cpp
	src/wnd/record_list_wnd.h
	src/wnd/record_wnd.cpp
	src/wnd/record_wnd.h
	src/wnd/settings_wnd.cpp
	src/wnd/settings_wnd.h
	src/wnd/setup_record_wnd.cpp
	src/wnd/setup_record_wnd.h
	src/wnd/wnd_base.cpp
	src/wnd/wnd_base.h
	resources/icon.rc
)

add_executable(DrugsAndPills WIN32 ${SOURCES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DrugsAndPills)

target_link_libraries(DrugsAndPills Comctl32.lib Msimg32.lib User32.lib)

target_include_directories(DrugsAndPills PRIVATE src src/wnd resources)

set_target_properties(DrugsAndPills PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_custom_command(TARGET DrugsAndPills POST_BUILD 
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico $<TARGET_FILE_DIR:DrugsAndPills>/resources/icon.ico
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon_warning.ico $<TARGET_FILE_DIR:DrugsAndPills>/resources/icon_warning.ico
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon_fail.ico $<TARGET_FILE_DIR:DrugsAndPills>/resources/icon_fail.ico
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon_atlas.png $<TARGET_FILE_DIR:DrugsAndPills>/resources/icon_atlas.png
	
)

target_compile_definitions(DrugsAndPills PRIVATE $<$<CONFIG:Release>:NDEBUG> _CONSOLE _UNICODE)