#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "eas.h"
#include "eas_report.h"

/* determines how many EAS buffers to fill a host buffer */
#define NUM_BUFFERS         8

/* default file to play if no filename is specified on the command line */
static const char defaultTestFile[] = "test.mid";

/* .WAV file format tags */
const EAS_UINT riffTag = 0x46464952;
const EAS_UINT waveTag = 0x45564157;
const EAS_UINT fmtTag = 0x20746d66;
const EAS_UINT dataTag = 0x61746164;

/* .WAV file format chunk */
typedef struct {
    EAS_U16 wFormatTag;
    EAS_U16 nChannels;
    EAS_UINT nSamplesPerSec;
    EAS_UINT nAvgBytesPerSec;
    EAS_U16 nBlockAlign;
    EAS_U16 wBitsPerSample;
} FMT_CHUNK;

/* .WAV file header */
typedef struct {
    EAS_UINT nRiffTag;
    EAS_UINT nRiffSize;
    EAS_UINT nWaveTag;
    EAS_UINT nFmtTag;
    EAS_UINT nFmtSize;
    FMT_CHUNK fc;
    EAS_UINT nDataTag;
    EAS_UINT nDataSize;
} WAVE_HEADER;

typedef struct {
    WAVE_HEADER wh;
    FILE *file;
    EAS_BOOL write;
    EAS_U32 dataSize;
} WAVE_FILE;


static int EAS_FILE_readAt(void *handle, void *buf, int offset, int size)
{
    if ( fseek( (FILE*) handle, offset, SEEK_SET ) != 0 )
        return -1;
    
    return fread( buf, 1, size, (FILE*) handle );
}

static int EAS_FILE_size( void *handle )
{
    FILE * fp = (FILE *) handle;
    long cur = ftell( fp );
    
    if ( fseek(fp, 0, SEEK_END ) != 0 )
        return 0;
    
    long size = ftell( fp );

    if ( fseek(fp, cur, SEEK_SET ) != 0 )
        return 0;
    
    return size;
}



#ifdef WORDS_BIGENDIAN
/*----------------------------------------------------------------------------
 * FlipDWord()
 *----------------------------------------------------------------------------
 * Purpose: Endian flip a DWORD for big-endian processors
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/
static void FlipDWord (EAS_U32 *pValue)
{
    EAS_U8 *p;
    EAS_U32 temp;

    p = (EAS_U8*) pValue;
    temp = (((((p[3] << 8) | p[2]) << 8) | p[1]) << 8) | p[0];
    *pValue = temp;
}

/*----------------------------------------------------------------------------
 * FlipWord()
 *----------------------------------------------------------------------------
 * Purpose: Endian flip a WORD for big-endian processors
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/
static void FlipWord (EAS_U16 *pValue)
{
    EAS_U8 *p;
    EAS_U16 temp;

    p = (EAS_U8*) pValue;
    temp = (p[1] << 8) | p[0];
    *pValue = temp;
}

/*----------------------------------------------------------------------------
 * FlipWaveHeader()
 *----------------------------------------------------------------------------
 * Purpose: Endian flip the wave header for big-endian processors
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/
static void FlipWaveHeader (WAVE_HEADER *p)
{

    FlipDWord(&p->nRiffTag);
    FlipDWord(&p->nRiffSize);
    FlipDWord(&p->nWaveTag);
    FlipDWord(&p->nFmtTag);
    FlipDWord(&p->nFmtSize);
    FlipDWord(&p->nDataTag);
    FlipDWord(&p->nDataSize);
    FlipWord(&p->fc.wFormatTag);
    FlipWord(&p->fc.nChannels);
    FlipDWord(&p->fc.nSamplesPerSec);
    FlipDWord(&p->fc.nAvgBytesPerSec);
    FlipWord(&p->fc.nBlockAlign);
    FlipWord(&p->fc.wBitsPerSample);

}
#endif

/*----------------------------------------------------------------------------
 * WaveFileCreate()
 *----------------------------------------------------------------------------
 * Purpose: Opens a wave file for writing and writes the header
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/

WAVE_FILE *WaveFileCreate (const char *filename, EAS_I32 nChannels, EAS_I32 nSamplesPerSec, EAS_I32 wBitsPerSample)
{
    WAVE_FILE *wFile;

    /* allocate memory */
    wFile = malloc(sizeof(WAVE_FILE));
    if (!wFile)
        return NULL;
    wFile->write = EAS_TRUE;

    /* create the file */
    wFile->file = fopen(filename,"wb");
    if (!wFile->file)
    {
        free(wFile);
        return NULL;
    }

    /* initialize PCM format .WAV file header */
    wFile->wh.nRiffTag = riffTag;
    wFile->wh.nRiffSize = sizeof(WAVE_HEADER) - 8;
    wFile->wh.nWaveTag = waveTag;
    wFile->wh.nFmtTag = fmtTag;
    wFile->wh.nFmtSize = sizeof(FMT_CHUNK);

    /* initalize 'fmt' chunk */
    wFile->wh.fc.wFormatTag = 1;
    wFile->wh.fc.nChannels = (EAS_U16) nChannels;
    wFile->wh.fc.nSamplesPerSec = (EAS_U32) nSamplesPerSec;
    wFile->wh.fc.wBitsPerSample = (EAS_U16) wBitsPerSample;
    wFile->wh.fc.nBlockAlign = (EAS_U16) (nChannels * (EAS_U16) (wBitsPerSample / 8));
    wFile->wh.fc.nAvgBytesPerSec = wFile->wh.fc.nBlockAlign * (EAS_U32) nSamplesPerSec;

    /* initialize 'data' chunk */
    wFile->wh.nDataTag = dataTag;
    wFile->wh.nDataSize = 0;

#ifdef WORDS_BIGENDIAN
    FlipWaveHeader(&wFile->wh);
#endif

    /* write the header */
    if (fwrite(&wFile->wh, sizeof(WAVE_HEADER), 1, wFile->file) != 1)
    {
        fclose(wFile->file);
        free(wFile);
        return NULL;
    }

#ifdef WORDS_BIGENDIAN
    FlipWaveHeader(&wFile->wh);
#endif

    /* return the file handle */
    return wFile;
} /* end WaveFileCreate */

/*----------------------------------------------------------------------------
 * WaveFileWrite()
 *----------------------------------------------------------------------------
 * Purpose: Writes data to the wave file
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/
EAS_I32 WaveFileWrite (WAVE_FILE *wFile, void *buffer, EAS_I32 n)
{
    EAS_I32 count;

    /* make sure we have an open file */
    if (wFile == NULL)
    {
        return 0;
    }

#ifdef WORDS_BIGENDIAN
    {
        EAS_I32 i;
        EAS_U16 *p;
        p = buffer;
        i = n >> 1;
        while (i--)
            FlipWord(p++);
    }
#endif

    /* write the data */
    count = (EAS_I32) fwrite(buffer, 1, (size_t) n, wFile->file);

    /* add the number of bytes written */
    wFile->wh.nRiffSize += (EAS_U32) count;
    wFile->wh.nDataSize += (EAS_U32) count;

    /* return the count of bytes written */
    return count;
} /* end WriteWaveHeader */

/*----------------------------------------------------------------------------
 * WaveFileClose()
 *----------------------------------------------------------------------------
 * Purpose: Opens a wave file for writing and writes the header
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/

EAS_BOOL WaveFileClose (WAVE_FILE *wFile)
{
    EAS_I32 count = 1;

    /* return to beginning of file and write the header */
    if (wFile->write)
    {
        if (fseek(wFile->file, 0L, SEEK_SET) == 0)
        {

#ifdef WORDS_BIGENDIAN
            FlipWaveHeader(&wFile->wh);
#endif
            count = (EAS_I32) fwrite(&wFile->wh, sizeof(WAVE_HEADER), 1, wFile->file);
#ifdef WORDS_BIGENDIAN
            FlipWaveHeader(&wFile->wh);
#endif
        }
    }

    /* close the file */
    if (fclose(wFile->file) != 0)
        count = 0;

    /* free the memory */
    free(wFile);

    /* return the file handle */
    return (count == 1 ? EAS_TRUE : EAS_FALSE);
} /* end WaveFileClose */


static EAS_BOOL EASLibraryCheck (const S_EAS_LIB_CONFIG *pLibConfig)
{
    // display the library version
    printf( "EAS Library Version %d.%d.%d.%d\n",
        pLibConfig->libVersion >> 24,
        (pLibConfig->libVersion >> 16) & 0x0f,
        (pLibConfig->libVersion >> 8) & 0x0f,
        pLibConfig->libVersion & 0x0f);

    // display some info about the library build
    if (pLibConfig->checkedVersion)
        printf( "\tChecked library\n");
    
    printf( "\tMaximum polyphony: %d\n", pLibConfig->maxVoices);
    printf( "\tNumber of channels: %d\n", pLibConfig->numChannels);
    printf( "\tSample rate: %d\n", pLibConfig->sampleRate);
    printf( "\tMix buffer size: %d\n", pLibConfig->mixBufferSize);
    
    if (pLibConfig->filterEnabled)
        printf( "\tFilter enabled\n");

    printf( "\tLibrary Build Timestamp: %s", ctime((time_t*)&pLibConfig->buildTimeStamp));
    printf( "\tLibrary Build ID: %s\n", pLibConfig->buildGUID);

    /* check it against the header file used to build this code */
    /*lint -e{778} constant expression used for display purposes may evaluate to zero */
    if (LIB_VERSION != pLibConfig->libVersion)
    {
        printf( "Library version does not match header files. EAS Header Version %d.%d.%d.%d\n",
            LIB_VERSION >> 24,
            (LIB_VERSION >> 16) & 0x0f,
            (LIB_VERSION >> 8) & 0x0f,
            LIB_VERSION & 0x0f);
        
        return EAS_FALSE;
    }
    
    return EAS_TRUE;
}



static EAS_RESULT PlayFile (EAS_DATA_HANDLE easData, const char* filename, const char* outputFile, const S_EAS_LIB_CONFIG *pLibConfig, void *buffer, EAS_I32 bufferSize)
{
    EAS_HANDLE handle;
    EAS_RESULT result, reportResult;
    EAS_I32 count;
    EAS_STATE state;
    EAS_I32 playTime;
    char waveFilename[256];
    WAVE_FILE *wFile;
    EAS_INT i;
    EAS_PCM *p;
    EAS_FILE file;

    // determine the name of the output file
    wFile = NULL;

    if (outputFile == NULL)
    {
        strncpy( waveFilename, filename, sizeof(waveFilename) );
        strncat( waveFilename, ".wav", sizeof(waveFilename) );
        outputFile = waveFilename;
    }

    // call EAS library to open file
    file.handle = fopen( filename, "rb" );
    file.size = EAS_FILE_size;
    file.readAt = EAS_FILE_readAt;
    
    if ( !filename )
    {
        fprintf( stderr, "Failed to open file %s: %s\n", filename, strerror(errno) );
        return EAS_FAILURE;
    }
    
    if ((reportResult = EAS_OpenFile(easData, &file, &handle)) != EAS_SUCCESS)
    {
        { fprintf( stderr, "EAS_OpenFile returned %ld\n", reportResult); }
        return reportResult;
    }

    // prepare to play the file
    if ((result = EAS_Prepare(easData, handle)) != EAS_SUCCESS)
    {
        { fprintf( stderr, "EAS_Prepare returned %ld\n", result); }
        reportResult = result;
    }

    // get play length
    if ((result = EAS_ParseMetaData(easData, handle, &playTime)) != EAS_SUCCESS)
    {
        { fprintf( stderr, "EAS_ParseMetaData returned %ld\n", result); }
        return result;
    }
    
    EAS_ReportEx(_EAS_SEVERITY_NOFILTER, 0xe624f4d9, 0x00000005 , playTime / 1000, playTime % 1000);

    if (reportResult == EAS_SUCCESS)
    {
        // create the output file
        wFile = WaveFileCreate(outputFile, pLibConfig->numChannels, pLibConfig->sampleRate, sizeof(EAS_PCM) * 8);
        
        if (!wFile)
        {
            fprintf( stderr, "Unable to create output file %s\n", waveFilename );
            reportResult = EAS_FAILURE;
        }
    }

    // rendering loop
    while (reportResult == EAS_SUCCESS)
    {
        // we may render several buffers here to fill one host buffer
        for (i = 0, p = buffer; i < NUM_BUFFERS; i++, p+= pLibConfig->mixBufferSize * pLibConfig->numChannels)
        {
            // get the current time
            if ((result = EAS_GetLocation(easData, handle, &playTime)) != EAS_SUCCESS)
            {
                fprintf( stderr, "EAS_GetLocation returned %d\n",result);
                
                if (reportResult == EAS_SUCCESS)
                    reportResult = result;
                
                break;
            }
            
            printf( "Parser time: %d.%03d\r", playTime / 1000, playTime % 1000 );

            // render a buffer of audio
            if ((result = EAS_Render(easData, p, pLibConfig->mixBufferSize, &count)) != EAS_SUCCESS)
            {
                fprintf( stderr, "EAS_Render returned %d\n",result);
                
                if (reportResult == EAS_SUCCESS)
                    reportResult = result;
            }
        }

        if ( result == EAS_SUCCESS )
        {
            // write it to the wave file
            if ( WaveFileWrite(wFile, buffer, bufferSize) != bufferSize )
            {
                fprintf( stderr, "WaveFileWrite failed\n");
                reportResult = EAS_FAILURE;
            }
        }

        if (reportResult == EAS_SUCCESS)
        {
            // check stream state
            if ((result = EAS_State(easData, handle, &state)) != EAS_SUCCESS)
            {
                fprintf( stderr, "EAS_State returned %d\n", result);
                reportResult = result;
            }

            // is playback complete
            if ( (state == EAS_STATE_STOPPED) || (state == EAS_STATE_ERROR) )
                break;
        }
    }

    printf("\nSuccess!\n");
    
    // close the output file
    if (wFile)
    {
        if (!WaveFileClose(wFile))
        {
            fprintf( stderr, "Error closing wave file %s\n", waveFilename);
            
            if (reportResult == EAS_SUCCESS)
                result = EAS_FAILURE;
        }
    }

    // close the input file
    if ((result = EAS_CloseFile(easData,handle)) != EAS_SUCCESS)
    {
        fprintf( stderr, "EAS_Close returned %ld\n", result);
        
        if (reportResult == EAS_SUCCESS)
            result = EAS_FAILURE;
    }

    return reportResult;
}

/*----------------------------------------------------------------------------
 * main()
 *----------------------------------------------------------------------------
 * Purpose: The entry point for the EAS sample application
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/
int main( int argc, char **argv )
{
    EAS_I32 polyphony;
    EAS_DATA_HANDLE easData;
    const S_EAS_LIB_CONFIG *pLibConfig;
    void *buffer;
    EAS_RESULT result, playResult;
    EAS_I32 bufferSize;
    int i;
    int temp;
    FILE *debugFile;
    char *outputFile = NULL;

    /* set the error reporting level */
    EAS_SetDebugLevel(_EAS_SEVERITY_INFO);
    debugFile = NULL;

    /* process command-line arguments */
    for (i = 1; i < argc; i++)
    {
        /* check for switch */
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
            case 'd':
                temp = argv[i][2];
                if ((temp >= '0') || (temp <= '9'))
                    EAS_SetDebugLevel(temp);
                break;
                
            case 'f':
                if ((debugFile = fopen(&argv[i][2],"w")) == NULL)
                    { fprintf( stderr, "Unable to create debug file %s\n", &argv[i][2]); }
                else
                    EAS_SetDebugFile(debugFile, EAS_TRUE);
                break;
            case 'o':
                outputFile = &argv[i][2];
                break;
            case 'p':
                polyphony = atoi(&argv[i][2]);
                if (polyphony < 1)
                    polyphony = 1;
                break;
                
            default:
                break;
            }
            continue;
        }
    }

    /* assume success */
    playResult = EAS_SUCCESS;

    /* get the library configuration */
    pLibConfig = EAS_Config();
    if (!EASLibraryCheck(pLibConfig))
        return -1;
    if (polyphony > pLibConfig->maxVoices)
        polyphony = pLibConfig->maxVoices;

    /* calculate buffer size */
    bufferSize = pLibConfig->mixBufferSize * pLibConfig->numChannels * (EAS_I32)sizeof(EAS_PCM) * NUM_BUFFERS;

    /* allocate output buffer memory */
    buffer = malloc((EAS_U32)bufferSize);
    if (!buffer)
    {
        { fprintf( stderr, "Error allocating memory for audio buffer\n"); }
        return EAS_FAILURE;
    }

    /* initialize the EAS library */
    polyphony = pLibConfig->maxVoices;
    if ((result = EAS_Init(&easData)) != EAS_SUCCESS)
    {
        { fprintf( stderr, "EAS_Init returned %ld - aborting!\n", result); }
        free(buffer);
        return result;
    }

    /*
     * Some debugging environments don't allow for passed parameters.
     * In this case, just play the default MIDI file "test.mid"
     */
    if (argc < 2)
    {
        { fprintf( stderr, "Playing '%s'\n", defaultTestFile);  }
        if ((playResult = PlayFile(easData, defaultTestFile, NULL, pLibConfig, buffer, bufferSize)) != EAS_SUCCESS)
        {
            { fprintf( stderr, "Error %d playing file %s\n", playResult, defaultTestFile); }
        }
    }
    /* iterate through the list of files to be played */
    else
    {
        for (i = 1; i < argc; i++)
        {
            /* check for switch */
            if (argv[i][0] != '-')
            {

                { fprintf( stderr, "Playing '%s'\n", argv[i]); }
                if ((playResult = PlayFile(easData, argv[i], outputFile, pLibConfig, buffer, bufferSize)) != EAS_SUCCESS)
                {
                    { fprintf( stderr, "Error %d playing file %s\n", playResult, argv[i]); }
                    break;
                }
            }
        }
    }

    /* shutdown the EAS library */
    if ((result = EAS_Shutdown(easData)) != EAS_SUCCESS)
    {
        { fprintf( stderr, "EAS_Shutdown returned %ld\n", result); }
    }

    /* free the output buffer */
    free(buffer);

    /* close the debug file */
    if (debugFile)
        fclose(debugFile);

    /* play errors take precedence over shutdown errors */
    if (playResult != EAS_SUCCESS)
        return playResult;
    return result;
} /* end main */
