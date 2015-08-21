#include "gdal_pam.h"

#include <iostream>
#include <boost/system/config.hpp>
#include <boost/filesystem.hpp>

CPL_CVSID("$Id: MDFdataset.cpp 27745 2014-09-27 16:38:57Z goatbar $");

CPL_C_START
void	GDALRegister_MDF(void);
CPL_C_END

/************************************************************************/
/*                            MDFGetField()                            */
/************************************************************************/

static int MDFGetField( char *pszField, int nWidth )

{
    char	szWork[32];

    CPLAssert( nWidth < (int) sizeof(szWork) );

    strncpy( szWork, pszField, nWidth );
    szWork[nWidth] = '\0';

    return atoi(szWork);
}

/************************************************************************/
/*                            MDFGetAngle()                            */
/************************************************************************/

static double MDFGetAngle( char *pszField )

{
    int		nAngle = MDFGetField( pszField, 7 );
    int		nDegree, nMin, nSec;

    // Note, this isn't very general purpose, but it would appear
    // from the field widths that angles are never negative.  Nice
    // to be a country in the "first quadrant". 

    nDegree = nAngle / 10000;
    nMin = (nAngle / 100) % 100;
    nSec = nAngle % 100;
    
    return nDegree + nMin / 60.0 + nSec / 3600.0;
}

/************************************************************************/
/* ==================================================================== */
/*				MDFDataset				*/
/* ==================================================================== */
/************************************************************************/

class MDFRasterBand;

class MDFDataset : public GDALPamDataset
{
    friend class MDFRasterBand;

    VSILFILE	*fp;
    GByte	abyHeader[1012];

  public:
                     MDFDataset();
                    ~MDFDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );
    static int Identify( GDALOpenInfo * );

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                            MDFRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class MDFRasterBand : public GDALPamRasterBand
{
    friend class MDFDataset;

    int          nRecordSize;
    char*        pszRecord;
    int          bBufferAllocFailed;
    
  public:

    		MDFRasterBand( MDFDataset *, int );
                ~MDFRasterBand();
    
    virtual CPLErr IReadBlock( int, int, void * );
};


/************************************************************************/
/*                           MDFRasterBand()                            */
/************************************************************************/

MDFRasterBand::MDFRasterBand( MDFDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;

    eDataType = GDT_Float32;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;

    /* Cannot overflow as nBlockXSize <= 999 */
    nRecordSize = nBlockXSize*5 + 9 + 2;
    pszRecord = NULL;
    bBufferAllocFailed = FALSE;
}

/************************************************************************/
/*                          ~MDFRasterBand()                            */
/************************************************************************/

MDFRasterBand::~MDFRasterBand()
{
    VSIFree(pszRecord);
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr MDFRasterBand::IReadBlock( CPL_UNUSED int nBlockXOff,
                                   int nBlockYOff,
                                   void * pImage )

{
    MDFDataset *poGDS = (MDFDataset *) poDS;
    int		i;
    
    if (pszRecord == NULL)
    {
        if (bBufferAllocFailed)
            return CE_Failure;

        pszRecord = (char *) VSIMalloc(nRecordSize);
        if (pszRecord == NULL)
        {
            CPLError(CE_Failure, CPLE_OutOfMemory,
                     "Cannot allocate scanline buffer");
            bBufferAllocFailed = TRUE;
            return CE_Failure;
        }
    }

    VSIFSeekL( poGDS->fp, 1011 + nRecordSize*nBlockYOff, SEEK_SET );

    VSIFReadL( pszRecord, 1, nRecordSize, poGDS->fp );

    if( !EQUALN((char *) poGDS->abyHeader,pszRecord,6) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "MDF Scanline corrupt.  Perhaps file was not transferred\n"
                  "in binary mode?" );
        return CE_Failure;
    }
    
    if( MDFGetField( pszRecord + 6, 3 ) != nBlockYOff + 1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "MDF scanline out of order, MDF driver does not\n"
                  "currently support partial datasets." );
        return CE_Failure;
    }

    for( i = 0; i < nBlockXSize; i++ )
        ((float *) pImage)[i] = (float)
            (MDFGetField( pszRecord + 9 + 5 * i, 5) * 0.1);

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*				MDFDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            MDFDataset()                             */
/************************************************************************/

MDFDataset::MDFDataset() : fp(NULL)

{
    fp = NULL;
}

/************************************************************************/
/*                           ~MDFDataset()                             */
/************************************************************************/

MDFDataset::~MDFDataset()

{
    FlushCache();
    if( fp != NULL )
        VSIFCloseL( fp );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr MDFDataset::GetGeoTransform( double * padfTransform )

{
    double	dfLLLat, dfLLLong, dfURLat, dfURLong;

    dfLLLat = MDFGetAngle( (char *) abyHeader + 29 );
    dfLLLong = MDFGetAngle( (char *) abyHeader + 36 );
    dfURLat = MDFGetAngle( (char *) abyHeader + 43 );
    dfURLong = MDFGetAngle( (char *) abyHeader + 50 );
    
    padfTransform[0] = dfLLLong;
    padfTransform[3] = dfURLat;
    padfTransform[1] = (dfURLong - dfLLLong) / GetRasterXSize();
    padfTransform[2] = 0.0;
        
    padfTransform[4] = 0.0;
    padfTransform[5] = -1 * (dfURLat - dfLLLat) / GetRasterYSize();


    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *MDFDataset::GetProjectionRef()

{
    return( "GEOGCS[\"Tokyo\",DATUM[\"Tokyo\",SPHEROID[\"Bessel 1841\",6377397.155,299.1528128,AUTHORITY[\"EPSG\",7004]],TOWGS84[-148,507,685,0,0,0,0],AUTHORITY[\"EPSG\",6301]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",8901]],UNIT[\"DMSH\",0.0174532925199433,AUTHORITY[\"EPSG\",9108]],AUTHORITY[\"EPSG\",4301]]" );
}

/************************************************************************/
/*                              Identify()                              */
/************************************************************************/

int MDFDataset::Identify( GDALOpenInfo * poOpenInfo )

{
/* -------------------------------------------------------------------- */
/*      Confirm that the header has what appears to be dates in the     */
/*      expected locations.  Sadly this is a relatively weak test.      */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 50 )
        return FALSE;
		
	

    /* check if century values seem reasonable */
    if( (!EQUALN((char *)poOpenInfo->pabyHeader+11,"19",2)
          && !EQUALN((char *)poOpenInfo->pabyHeader+11,"20",2))
        || (!EQUALN((char *)poOpenInfo->pabyHeader+15,"19",2)
             && !EQUALN((char *)poOpenInfo->pabyHeader+15,"20",2))
        || (!EQUALN((char *)poOpenInfo->pabyHeader+19,"19",2)
             && !EQUALN((char *)poOpenInfo->pabyHeader+19,"20",2)) )
    {
        return FALSE;
    }

    return TRUE;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *MDFDataset::Open( GDALOpenInfo * poOpenInfo )

{
/* -------------------------------------------------------------------- */
/*      Confirm that the header is compatible with a MDF dataset.      */
/* -------------------------------------------------------------------- */
    if (!Identify(poOpenInfo))
        return NULL;

/* -------------------------------------------------------------------- */
/*      Confirm the requested access is supported.                      */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->eAccess == GA_Update )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The MDF driver does not support update access to existing"
                  " datasets.\n" );
        return NULL;
    }
    
    /* Check that the file pointer from GDALOpenInfo* is available */
    if( poOpenInfo->fpL == NULL )
    {
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    MDFDataset 	*poDS;

    poDS = new MDFDataset();

    /* Borrow the file pointer from GDALOpenInfo* */
    poDS->fp = poOpenInfo->fpL;
    poOpenInfo->fpL = NULL;
    
/* -------------------------------------------------------------------- */
/*      Read the header.                                                */
/* -------------------------------------------------------------------- */
    VSIFReadL( poDS->abyHeader, 1, 1012, poDS->fp );

    poDS->nRasterXSize = MDFGetField( (char *) poDS->abyHeader + 23, 3 );
    poDS->nRasterYSize = MDFGetField( (char *) poDS->abyHeader + 26, 3 );
    if  (poDS->nRasterXSize <= 0 || poDS->nRasterYSize <= 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Invalid dimensions : %d x %d", 
                  poDS->nRasterXSize, poDS->nRasterYSize); 
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->SetBand( 1, new MDFRasterBand( poDS, 1 ));

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

/* -------------------------------------------------------------------- */
/*      Check for overviews.                                            */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return( poDS );
}

/************************************************************************/
/*                          GDALRegister_MDF()                          */
/************************************************************************/

void GDALRegister_MDF()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "MDF" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "MDF" );
        poDriver->SetMetadataItem( GDAL_DCAP_RASTER, "YES" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Jerry MyDataFormat (.mdf)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#MDF" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "mdf" );
        poDriver->SetMetadataItem( GDAL_DCAP_VIRTUALIO, "YES" );

        poDriver->pfnOpen = MDFDataset::Open;
        poDriver->pfnIdentify = MDFDataset::Identify;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
