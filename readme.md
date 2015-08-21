# Building
See https://trac.osgeo.org/gdal/wiki/BuildingOnWindows


##Windows
 ___Aug 21/2015___ VC2013 works but 2015 has issues with some odbc linking issue missing exports so we are assuming VS2013
 
####Building x64
   * start your VS2013 x64 Native Tools Command Prompt
   * goto <root>\gdal\gdal
   * `nmake /f makefile.vc MSVC_VER=1800 WIN64=YES install`

#### Dependancies
 * www.boost.org
   1. download latest version [1_59_0 as of Aug21/2015](http://sourceforge.net/projects/boost/files/boost/1.59.0/)
   2. run ```bootstrap.bat```
   3. run ```b2.exe --stagedir=./stage/x64 --build-type=complete --layout=versioned variant=release link=shared runtime-link=shared threading=multi toolset=msvc-12.0 address-model=64 stage```
   <pre>
b2.exe --stagedir=./stage/x64 --build-type=complete --layout=versioned variant=release link=shared runtime-link=shared threading=multi toolset=msvc-12.0 address-model=64 stage
b2.exe --stagedir=./stage/x32 --build-type=complete --layout=versioned variant=release link=shared runtime-link=shared threading=multi toolset=msvc-12.0 stage
b2.exe --stagedir=./stage/x64 --build-type=complete --layout=versioned variant=release link=static threading=multi toolset=msvc-12.0 address-model=64 stage
b2.exe --stagedir=./stage/x32 --build-type=complete --layout=versioned variant=release link=static threading=multi toolset=msvc-12.0 stage
   </pre>
 
#### Adding new driver (using mdf as example)
 * add code to <root>\gdal\gdal\frmts\mdf
     <pre>
 ├───mdf
 │       GNUmakefile
 │       makefile.vc
 │       mdfdataset.cpp
     </pre>
 * goto <root>\gdal\gdal
   * edit ```nmake.opt```
     <pre>
 BOOST_VERSION = 1_59
 BOOST_PATH = D:/3rd-party/boost.org/boost_1_59_0
 BOOST_LIBS = D:/3rd-party/boost.org/boost_1_59_0/stage/x64/lib
 MDF_CFLAGS = -I$(BOOST_PATH)
 MDF_LIBS = $(BOOST_LIBS)/boost_system-vc120-mt-1_59.lib $(BOOST_LIBS)/boost_filesystem-vc120-mt-1_59.lib
     </pre>
     <pre>
 ADD_LIBS	= $(MDF_LIBS)
     </pre>
     <pre>
 CFLAGS	= -DBOOST_ALL_NO_LIB ...
     </pre>
   * edit ```<root>\gdal\gdal\frmts\makefile.vc```
     <pre>
     EXTRAFLAGS =	... -DFRMT_mdf
     </pre>
   * edit ```<root>\gdal\gdal\frmts\gdalallregister.cpp``` add below last ___#endif___ just above ` GetGDALDriverManager()->AutoSkipDrivers();`
     <pre>
     #ifdef FRMT_mdf
         GDALRegister_MDF();
     #endif
     </pre>
   * follow the steps as in ___Building x64___
