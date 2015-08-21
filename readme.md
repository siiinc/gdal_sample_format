# Building
___Aug 21/2015___ VC2013 works but 2015 has issues with some odbc linking issue missing exports
* goto <root>\gdal\gdal
  * Windows nmake command
    * start your VS2013 x64 Native Tools Command Prompt
    * `nmake /f makefile.vc MSVC_VER=1800 WIN64=YES install`
  * Linux ./configure
    * todo

# New driver (using mdf as example)
* add code to <root>\gdal\gdal\frmts\mdf
    <pre>
├───mdf
│       GNUmakefile
│       makefile.vc
│       mdfdataset.cpp
    </pre>
* goto <root>\gdal\gdal
  * edit nmake.opt
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
  * `<root>\gdal\gdal\frmts\makefile.vc`
    <pre>
    EXTRAFLAGS =	... -DFRMT_mdf
    </pre>
  * `<root>\gdal\gdal\frmts\gdalallregister.cpp` add below last ___#endif___ just above ` GetGDALDriverManager()->AutoSkipDrivers();`
    <pre>
    #ifdef FRMT_mdf
        GDALRegister_MDF();
    #endif
    </pre>
