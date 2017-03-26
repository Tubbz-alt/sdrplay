#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mirsdrapi-rsp.h>


void usage()
{
    fprintf(stderr, "usage: sdrtest [-f frequency][-s samplerate][-b bw][-i if][-g gR][-L LNA][-a ANT][-r duration][-l][-d device][-v]\n");
    fprintf(stderr, "  -f frequency    set frequency in MHz\n");
    fprintf(stderr, "  -s samplerate   set samplerate in MHz\n");
    fprintf(stderr, "  -b bandwidth    set bw in kHz: 200, 300, 600, 1536, 5000, 6000, 7000, 8000\n");
    fprintf(stderr, "  -i if-frequency set IF in kHz: 0, 450, 1620, 2048\n");
    fprintf(stderr, "  -g gR           set gain Reduction in dB 0 .. 102\n");
    fprintf(stderr, "  -L LNA          set LNA state. RSP-1: 0..3. RSP-2: 0..8 depends on frequency\n");
    fprintf(stderr, "  -a ANT          use Antenna 'A' or 'B' - RSP-2 only\n");

    fprintf(stderr, "  -r duration     set record duration in seconds\n");
    fprintf(stderr, "  -d device       set device s/n or idx to use\n");
    fprintf(stderr, "  -l              list devices\n");
    fprintf(stderr, "  -v            print version and exit\n");
}

const char * mirErrTxt[] = {
  "Success",
  "Fail",
  "InvalidParam",
  "OutOfRange",
  "GainUpdateError",
  "RfUpdateError",
  "FsUpdateError",
  "HwError",
  "AliasingError",
  "AlreadyInitialised",
  "NotInitialised",
  "NotEnabled",
  "HwVerError",
  "OutOfMemError"
};


bool dbgRet( mir_sdr_ErrT rc, const char * f ) {
  if ( mir_sdr_Success == rc )
    fprintf( stderr, "%s returned %s (=%d)\n", f, mirErrTxt[rc], (int)rc );
  else if ( mir_sdr_Success < rc && rc <= mir_sdr_OutOfMemError )
    fprintf( stderr, "%s returned error %s (=%d)\n", f, mirErrTxt[rc], (int)rc );
  else
    fprintf( stderr, "%s returned error code %d\n", f, (int)rc );
  return ( mir_sdr_Success == rc );
}

static volatile unsigned int sumN = 0;

void sdr_StreamCallback(short *xi, short *xq, unsigned int firstSampleNum, int grChanged, int rfChanged, int fsChanged, unsigned int numSamples, unsigned int reset, void *cbContext)
{
  if ( grChanged || rfChanged || fsChanged || reset )
    fprintf(stderr, "sumN %u: sdr_StreamCallback( N %u, 1st %u, grChanged %d, rfChanged %d, fsChanged %d, reset %u )\n",
      sumN, numSamples, firstSampleNum, grChanged, rfChanged, fsChanged, reset );
  else
    fprintf(stderr, "sumN %u: sdr_StreamCallback( N %u, 1st %u )\n",
      sumN, numSamples, firstSampleNum );

  sumN += numSamples;

  static unsigned int xclen = 0;
  static short * xc = 0;
  if ( xclen < numSamples )
  {
    delete []xc;
    xc = new short[ 2 * numSamples ];
    xclen = numSamples;
  }

  unsigned int off = 0;
  for ( unsigned int i = 0; i < numSamples; ++i ) {
    xc[off++] = xi[i];
    xc[off++] = xq[i];
  }

  fwrite( xc, 2*sizeof(short), numSamples, stdout );
}

void sdr_GainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB, void *cbContext)
{
  fprintf(stderr, "sumN %u: sdr_GainChangeCallback( gRdB %u, lnaGRdb %u )\n", sumN, gRdB, lnaGRdB );
}


int main(int argc, char **argv)
{
    mir_sdr_DeviceT devices[4];
    unsigned int numDevs = 0;
    unsigned int selDevIdx = 5;
    unsigned int devno = 0;

    int c;
    double rfMHz = 105.2;
    double fsMHz = 2.048;
    mir_sdr_RSPII_AntennaSelectT antSelect = mir_sdr_RSPII_ANTENNA_A;
    mir_sdr_Bw_MHzT bwType = mir_sdr_BW_1_536;
    mir_sdr_If_kHzT ifType = mir_sdr_IF_Zero;
    int gRdB = 0;
    int LNAstate = 0;
    int recDurationSecs = 1;

    mir_sdr_ErrT retSet;
    mir_sdr_ErrT retGetDevs = mir_sdr_GetDevices(&devices[0], &numDevs, 4 );
    dbgRet( retGetDevs, "mir_sdr_GetDevices()" );

    while((c = getopt(argc, argv, "f:s:b:i:g:L:a:r:d:lv")) > 0) {
        switch(c)
        {
            case 'f':
                rfMHz = atof(optarg);
                break;
            case 's':
                fsMHz = atof(optarg);
                break;
            case 'b':
                bwType = (mir_sdr_Bw_MHzT)atoi(optarg);
                break;
            case 'i':
                ifType = (mir_sdr_If_kHzT)atoi(optarg);
                break;
            case 'g':
                gRdB = atoi(optarg);
                break;
            case 'L':
                LNAstate = atoi(optarg);
                break;
            case 'a':
                if (!strcmp("A",optarg) || !strcmp("a",optarg) )
                    antSelect = mir_sdr_RSPII_ANTENNA_A;
                else if (!strcmp("B",optarg) || !strcmp("b",optarg) )
                    antSelect = mir_sdr_RSPII_ANTENNA_B;
                break;

            case 'r':
                recDurationSecs = atoi(optarg);
                break;
            case 'd':
                for( devno = 0; devno < numDevs; ++devno) {
                    if ( !strcmp( devices[devno].SerNo, optarg ) ) {
                        fprintf(stderr, "selected device sn %u: sn '%s', nm '%s', hwVer %d, avail %d\n",
                            devno,
                            devices[devno].SerNo,
                            devices[devno].DevNm,
                            (int)devices[devno].hwVer,
                            (int)devices[devno].devAvail
                            );
                        selDevIdx = devno;
                        break;
                    }
                }
                if ( numDevs <= selDevIdx ) {
                    selDevIdx = devno = atoi(optarg);
                    fprintf(stderr, "selected deviceno %u: sn '%s', nm '%s', hwVer %d, avail %d\n",
                        devno,
                        devices[devno].SerNo,
                        devices[devno].DevNm,
                        (int)devices[devno].hwVer,
                        (int)devices[devno].devAvail
                        );
                }
                retSet = mir_sdr_SetDeviceIdx(selDevIdx);
                dbgRet( retSet, "mir_sdr_SetDeviceIdx()" );
                break;
            case 'l':
                fprintf(stderr, "found %u devices:\n", numDevs);
                for( devno = 0; devno < numDevs; ++devno)
                    fprintf(stderr, "%u: sn '%s', nm '%s', hwVer %d, avail %d\n",
                        devno,
                        devices[devno].SerNo,
                        devices[devno].DevNm,
                        (int)devices[devno].hwVer,
                        (int)devices[devno].devAvail
                        );
                return 0;
            case 'v':
                fprintf(stderr, "SDRtest version ?\n");
                return 0;
            case 'h':
            default:
                usage();
                return -1;
        }
    }

    int gRdBsystem = 0;
    //mir_sdr_SetGrModeT setGrMode = mir_sdr_USE_RSP_SET_GR;
    mir_sdr_SetGrModeT setGrMode = mir_sdr_USE_SET_GR;

    int samplesPerPacket = 512;

    mir_sdr_ErrT retAntSel = mir_sdr_RSPII_AntennaControl(antSelect);
    dbgRet( retAntSel, "mir_sdr_RSPII_AntennaControl()" );

    mir_sdr_ErrT retInit = mir_sdr_StreamInit( &gRdB, fsMHz, rfMHz, bwType, ifType, LNAstate, &gRdBsystem,
        setGrMode, &samplesPerPacket, sdr_StreamCallback, sdr_GainChangeCallback, 0 );
    dbgRet( retInit, "mir_sdr_StreamInit()" );
    fprintf( stderr, "mir_sdr_StreamInit() => gRdB %d, gRdBsystem %d, samplesPerPacket %d\n"
        , gRdB, gRdBsystem, samplesPerPacket );

    sleep(recDurationSecs);
    fprintf(stderr, "sumN %u: calling mir_sdr_SetGr( gRdB + 10 )\n", sumN );
    mir_sdr_ErrT retSetGr = mir_sdr_SetGr( gRdB + 10, 1/*=abs*/, 1/*=syncUpdate*/ );
    dbgRet( retSetGr, "mir_sdr_SetGr( +10 )" );
    if ( mir_sdr_Success != retSetGr ) {
        gRdB = gRdB + 10;
        mir_sdr_ErrT retReInit = mir_sdr_Reinit(&gRdB, fsMHz, rfMHz, bwType, ifType, mir_sdr_LO_Auto,
          LNAstate, &gRdBsystem, setGrMode, &samplesPerPacket, mir_sdr_CHANGE_GR );
        dbgRet( retReInit, "mir_sdr_Reinit( gR +10 )" );
    }

    sleep(recDurationSecs);
    fprintf(stderr, "sumN %u: calling mir_sdr_SetRf( rfMHz + 1.0 M )\n", sumN );
    mir_sdr_ErrT retSetRf = mir_sdr_SetRf( rfMHz+1.0, 1/*=abs*/, 1/*=syncUpdate*/ );
    dbgRet( retSetRf, "mir_sdr_SetRf( + 1 MHz )" );
    if ( mir_sdr_Success != retSetRf ) {
        mir_sdr_ErrT retReInit = mir_sdr_Reinit(&gRdB, fsMHz, rfMHz+1.0, bwType, ifType, mir_sdr_LO_Auto,
          LNAstate, &gRdBsystem, setGrMode, &samplesPerPacket, mir_sdr_CHANGE_RF_FREQ );
        dbgRet( retReInit, "mir_sdr_Reinit( rf + 1 M )" );
    }

    sleep(recDurationSecs);
    fprintf(stderr, "sumN %u: calling mir_sdr_SetFs( fsMHz + 1.0 M )\n", sumN );
    mir_sdr_ErrT retSetFs = mir_sdr_SetFs( fsMHz+1.0, 1/*=abs*/, 1/*=syncUpdate*/, 0/*=reCal*/);
    dbgRet( retSetFs, "mir_sdr_SetFs( + 1 MHz)" );
    if ( mir_sdr_Success != retSetFs ) {
        mir_sdr_ErrT retReInit = mir_sdr_Reinit(&gRdB, fsMHz+1.0, rfMHz+1.0, bwType, ifType, mir_sdr_LO_Auto,
          LNAstate, &gRdBsystem, setGrMode, &samplesPerPacket, mir_sdr_CHANGE_FS_FREQ );
        dbgRet( retReInit, "mir_sdr_Reinit( fs + 1 M )" );
    }

    sleep(recDurationSecs);
    mir_sdr_StreamUninit();

    return 0;
}

