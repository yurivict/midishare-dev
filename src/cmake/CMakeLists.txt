#######################################
# CMAKE midishare
#######################################
project(midishare)
cmake_minimum_required(VERSION 2.6)

#######################################
#  Android support
if(ANDROID)
	message (STATUS "Setting android-based variables")
	set (CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/android.toolchain.cmake)
	set (STATICLIB "yes")
endif(ANDROID)

#######################################
if(WIN32)
 if (${CMAKE_GENERATOR} MATCHES "MSYS*")
	if (ARCH STREQUAL "64")
		message (STATUS "64 bits architecture will be generated - Use -DARCH=32 to change.")
		add_definitions(-m64)
		set (target mshare64)	
	else (ARCH STREQUAL "64")
		message (STATUS "32 bits architecture will be generated - Use -DARCH=64 to change.")
		set (target mshare32)
	endif (ARCH STREQUAL "64")
 else ()
	 if (${CMAKE_GENERATOR} MATCHES ".*Win64")
		set (target mshare64)
		set (X64 -D__x86_64__)
	 else ()
		set (target mshare32)
	 endif ()
 endif ()
else(WIN32)
	set (target MidiShare)
endif(WIN32)

#######################################
# versions management
set (VERSION B)
set (STRVERS "v.1.93")
set (SSTRVERS "1.93")

if(UNIX)
	add_definitions(-Wall -DGCC)
endif(UNIX)

if(ANDROID)
 	add_definitions(-Dandroid)
elseif(APPLE)
	set (CMAKE_OSX_ARCHITECTURES "x86_64 i386")
 	add_definitions(-D__Macintosh__ -D__MacOSX__)
	set (CMAKE_C_FLAGS -mmacosx-version-min=10.6)
endif()

if(WIN32)
	add_definitions(-DWINVER=0x0400 -DWIN32 -DMSEXPORT ${X64})
endif(WIN32)

#######################################
# Options disabled by default
option ( NODRIVERS 	"compiles midishare without drivers support" off )

if (NODRIVERS)
	message (STATUS "Generates the midishare kernel without drivers support - use -DNODRIVERS=off to change")
	add_definitions(-DNODRIVERS)
else()
	message (STATUS "Generates the midishare kernel with drivers support - use -DNODRIVERS=on to change")
endif()

#######################################
# set directories, src and headers.
set (ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/..)
set (SRC_PATH  ${ROOT_PATH})
set (TOOLS_PATH  ${ROOT_PATH}/mstools)
set (TESTS_PATH  ${ROOT_PATH}/tools)
set (LIB_PATH  ${ROOT_PATH}/lib)
set (SRCFOLDERS  common/Clients common/Kernel common/Drivers common/Memory  common/Sorter)

set (HEADERS_PATH ${SRC_PATH}/common/Headers ${SRC_PATH}/common/atomic ${SRC_PATH}/lib)
if(WIN32)
	set(IMPL_PATH ${ROOT_PATH}/win-mono)
elseif(APPLE)
	set(IMPL_PATH ${ROOT_PATH}/macosx-mono)
elseif(UNIX)
	set(IMPL_PATH ${ROOT_PATH}/linux)
endif(WIN32)


#######################################
# add src and headers files
foreach(folder ${SRCFOLDERS})
	set (SRC ${SRC} "${SRC_PATH}/${folder}/*.c")					# add source files
endforeach(folder)
set (SRC ${SRC} "${SRC_PATH}/common/atomic/lffifo.c"  "${SRC_PATH}/common/atomic/lflifo.c")

if(WIN32)
	if (${CMAKE_GENERATOR} MATCHES ".*Win64")
		set (SRC ${SRC} "${SRC_PATH}/common/atomic/msAtomicWin32.obj")
	endif (${CMAKE_GENERATOR} MATCHES ".*Win64")

	set (SRC ${SRC} "${IMPL_PATH}/*.c")					# add source files
elseif(APPLE)
	set (SRC ${SRC} "${IMPL_PATH}/*.c"  "${SRC_PATH}/lib/msPrefs.c" "${SRC_PATH}/lib/profport.c")
	set (HEADERS_PATH ${HEADERS_PATH} ${SRC_PATH}/lib)
elseif(UNIX)
	set (SRC ${SRC} "${IMPL_PATH}/library*.c" "${SRC_PATH}/lib/profport.c")		# add source files
endif(WIN32)
file (GLOB LIBSRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRC})

set (PUBLICHEADERS ${SRC_PATH}/include)

foreach(folder ${HEADERS_PATH})
	set(HEADERS ${HEADERS} "${folder}/*.h")		# add header files
endforeach(folder)
file (GLOB PUBLICHEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${PUBLICHEADERS}/*.h")
file (GLOB LIBH RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${HEADERS})

#######################################
# set includes
foreach(folder ${HEADERS_PATH})
	set(INCL ${INCL} "${folder}")				# add include folders
endforeach(folder)

include_directories( ${INCL})
set_source_files_properties (${INCL} PROPERTIES HEADER_FILE_ONLY TRUE)

#######################################
# set input libraries
if(APPLE)
	set(LINK "-framework CoreAudio")
endif(APPLE)

if(WIN32)
if (MSVC)
	set(LINK "winmm.lib")
else (MSVC)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(LINK "-lwinmm")
endif (MSVC)
endif(WIN32)

#######################################
# set library target
if(WIN32)
if (MSVC)
	enable_language(RC)
	set(LIBCONTENT ${LIBSRC} ${LIBH} ${PUBLICHEADERS} ${ROOT_PATH}/win-mono/midishare.rc)
	set(LINK_FLAGS /ENTRY:"DllEntryPoint")
else (MSVC)
	set(LIBCONTENT ${LIBSRC} ${LIBH} ${PUBLICHEADERS})
endif (MSVC)
else(WIN32)
	set(LIBCONTENT ${LIBSRC} ${LIBH} ${PUBLICHEADERS})
endif(WIN32)

if (STATICLIB)
	add_library(${target}-static STATIC ${LIBCONTENT})
	set_target_properties (${target}-static PROPERTIES
			output_name libmidishare.a)
endif()

add_library(${target} SHARED ${LIBCONTENT})
set_target_properties (${target} PROPERTIES
			FRAMEWORK TRUE
			VERSION ${VERSION}
			FRAMEWORK_VERSION ${VERSION}
			PUBLIC_HEADER "${PUBLICHEADERS}"
			MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${STRVERS}
			MACOSX_FRAMEWORK_BUNDLE_VERSION ${SSTRVERS}
)
if(WIN32 AND MSVC)
 set_target_properties (${target} PROPERTIES LINK_FLAGS /ENTRY:DllEntryPoint)
elseif(APPLE)
 set_target_properties (${target} PROPERTIES LINK_FLAGS -flat_namespace)
endif(WIN32 AND MSVC)

target_link_libraries (${target} ${LINK})


#######################################
# set tools targets
set (MSTOOLS MidiGetIndAppl MidiGetTime MidiIsConnected MidiConnect MidiGetName MidiGetVersion
MidiCountAppls MidiGetNamedAppl)

include_directories( ${SRC_PATH}/include)

foreach(tool ${MSTOOLS})
	add_executable( ${tool} ${TOOLS_PATH}/${tool}.c )
	target_link_libraries( ${tool} ${target})
	add_dependencies(${tool} ${target})
endforeach(tool)

#######################################
# set test targets
set (MSTESTS TFonctions TEvents TFilters TDrivers)
if (NOT NODRIVERS)
	set (MSTESTS  ${MSTESTS} TOutEvents)
endif()

include_directories( ${SRC_PATH}/include)

foreach(tool ${MSTESTS})
	add_executable( ${tool} ${TESTS_PATH}/${tool}.c )
	target_link_libraries( ${tool} ${target})
	add_dependencies(${tool} ${target})
endforeach(tool)

add_executable( TLin ${TESTS_PATH}/TLin.c ${LIB_PATH}/EventToMidiStream.c ${LIB_PATH}/MidiStreamToEvent.c)
target_link_libraries( TLin ${target})
add_dependencies(TLin ${target})
