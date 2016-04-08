project( libheekscnc )
cmake_minimum_required( VERSION 2.6 )
if( NOT DEFINED BUILD_STARTED_IN_RIGHT_DIR )
  message( FATAL_ERROR "Don't start a build in the src directory. Do a 'cd ..', then run cmake ." )
endif( NOT DEFINED BUILD_STARTED_IN_RIGHT_DIR )

set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DHEEKSPLUGIN -DHEEKSCNC -DUNICODE -DTIXML_USE_STL
                  -DOPEN_SOURCE_GEOMETRY -DWXUSINGDLL )
find_package( OpenGL REQUIRED )
find_package( wxWidgets REQUIRED COMPONENTS core gl aui stc )

#find OCE or OpenCASCADE
set( CASCADE_LIBS "TKernel;TKBRep;TKTopAlgo;TKMath;TKV3d;TKGeomBase;TKGeomAlgo;TKShHealing;TKBO;TKBool;TKOffset;TKLCAF;TKMath;TKService" )
#inherits variables from parent dir - don't need to 'find_package ( OCE )' again
if( ${OCE_FOUND} )
  set( OpenCASCADE_LIBRARIES ${CASCADE_LIBS} )
  set( OpenCASCADE_INCLUDE_DIR ${OCE_INCLUDE_DIRS} )
else() #look for OpenCASCADE
  set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
  find_package ( OpenCASCADE REQUIRED COMPONENTS ${CASCADE_LIBS} )
endif()

include(${wxWidgets_USE_FILE})

include_directories (
    ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${wxWidgets_INCLUDE_DIRS} ${OpenCASCADE_INCLUDE_DIR}
    ${HeeksCadDir} ../kurve/geometry ../kurve/messages .
    ${OPENGL_INCLUDE_DIR}
    )
set ( LIBRARY_OUTPUT_PATH ../bin )

if( UNIX )
  #set( platform_SRCS some_necessary_linux_file.cpp )
  add_definitions( -DCMAKE_UNIX ) #defining this changes resource and script locations
else( UNIX )
  #windows
  set( platform_SRCS dllmain.cpp stdafx.cpp )
endif( UNIX )

set( heekscnc_HDRS
    CNCConfig.h
    CNCPoint.h
    CTool.h
    CToolDlg.h
    DepthOp.h
    DepthOpDlg.h
    Drilling.h
    DrillingDlg.h
    DropCutter.h
    Excellon.h
    GTri.h
    HeeksCNC.h
    HeeksCNCInterface.h
    HeeksCNCTypes.h
    Interface.h
    NCCode.h
    Op.h
    OpDlg.h
    Operations.h
    OutputCanvas.h
    Pattern.h
    PatternDlg.h
    Pocket.h
    PocketDlg.h
    Profile.h
    ProfileDlg.h
    Program.h
    ProgramCanvas.h
    ProgramDlg.h
    PythonString.h
    PythonStuff.h
    Reselect.h
    ScriptOp.h
    ScriptOpDlg.h
    Simulate.h
    SketchOp.h
    SketchOpDlg.h
    SolidsDlg.h
    SpeedOp.h
    SpeedOpDlg.h
    Stock.h
    StockDlg.h
    Stocks.h
    Surface.h
    SurfaceDlg.h
    Surfaces.h
    Tag.h
    Tags.h
    Tools.h
    TrsfNCCode.h
    stdafx.h
    )

set( heekscnc_SRCS
    CNCPoint.cpp
    CTool.cpp
    CToolDlg.cpp
    DepthOp.cpp
    DepthOpDlg.cpp
    Drilling.cpp
    DrillingDlg.cpp
    DropCutter.cpp
    Excellon.cpp
    HeeksCNC.cpp
    HeeksCNCInterface.cpp
    Interface.cpp
    NCCode.cpp
    Op.cpp
    OpDlg.cpp
    Operations.cpp
    OutputCanvas.cpp
    Pattern.cpp
    PatternDlg.cpp
    Patterns.cpp
    Pocket.cpp
    PocketDlg.cpp
    Profile.cpp
    ProfileDlg.cpp
    Program.cpp
    ProgramCanvas.cpp
    ProgramDlg.cpp
    PythonString.cpp
    PythonStuff.cpp
    Reselect.cpp
    ScriptOp.cpp
    ScriptOpDlg.cpp
    Simulate.cpp
    SketchOp.cpp
    SketchOpDlg.cpp
    SolidsDlg.cpp
    SpeedOp.cpp
    SpeedOpDlg.cpp
    Stock.cpp
    StockDlg.cpp
    Stocks.cpp
    Surface.cpp
    SurfaceDlg.cpp
    Surfaces.cpp
    Tag.cpp
    Tags.cpp
    Tools.cpp
    TrsfNCCode.cpp
   )


set( HeeksCAD_LIBS
       -L${HeeksCadDir}/bin -ltinyxml -lheeksintf
   )

add_library( heekscnc SHARED ${heekscnc_SRCS} ${platform_SRCS} ${heekscnc_HDRS} )
target_link_libraries( heekscnc ${HeeksCAD_LIBS} ${wxWidgets_LIBRARIES} ${OpenCASCADE_LIBRARIES} ${OSX_LIBS} )
set_target_properties( heekscnc PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
# set_target_properties( heekscnc PROPERTIES LINK_FLAGS )

#---------------- the lines below tell cmake what files get installed where.---------------------
#------------------- this is used for 'make install' and 'make package' -------------------------
install( TARGETS heekscnc DESTINATION lib )

install( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bitmaps/" DESTINATION share/heekscnc/bitmaps/ PATTERN .svn EXCLUDE )

file( GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*.png" )
install( FILES ${icons} DESTINATION share/heekscnc/icons )

file(GLOB nc "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.txt")
install(FILES ${nc} DESTINATION lib/heekscnc/nc )

file( GLOB hcnc_defaults
               "${CMAKE_CURRENT_SOURCE_DIR}/../default.*"
               )
install( FILES ${hcnc_defaults} DESTINATION lib/heekscnc/ )

