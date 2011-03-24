#include "ztebladefm.h"

// Any functions in here that handles the input commands, we do *know* they have been validated anyway so dud input is impossible! :)
extern int g_keepRunning;
extern struct Commands cmds[CMDS_LEN];

static const char *delimiters = " ,\t";

#define TUNE_IDX        2
#define VOLUME_IDX      3
#define AUDIO_IDX       4
#define BAND_IDX        5
#define TUNESTEP_IDX    6
#define AUTOSEEK_IDX    7

void handle_on(const char *buf){
    int rv = 0, status = 0;
#ifdef __t0mm13b_defiant__
    printf("Handle Radio ON!\n");
#else
    // Since this is ON
    rv = ioctl(g_rdfmdev, FM_INIT2NORMAL, &status);
#endif
    fprintf(g_fpwrstatpipe, "[FM_INIT2NORMAL] - rv = %d; status = %d\n", rv, status);
}

void handle_off(const char *buf){
    int rv = 0, status = 0;
#ifdef __t0mm13b_defiant__
    printf("Handle Radio OFF!\n");
#else
    // Since this is OFF
    rv = ioctl(g_rdfmdev, FM_NORMAL2STANDBY, &status);
#endif
    fprintf(g_fpwrstatpipe, "[FM_NORMAL2STANDBY] - rv = %d; status = %d\n", rv, status);
}

void handle_frequency(const char *buf){
    int rv = 0, status = 0;
#ifdef __t0mm13b_defiant__
    printf("Handle Radio Frequency!\n");
#else
	rv = ioctl(fd, FM_GET_CURRENTFREQ, &status);
#endif	
	fprintf(g_fpwrstatpipe, "[FM_GETCURRENTFREQ] - rv = %d; status = %d\n", rv, status);
}

void handle_reset(const char *buf){
    const char *reset_stg1 = "BAND GENERIC ";
    const char *reset_stg2 = "AUTOSEEK UP 0 ";
    const char *reset_stg3 = "TUNESTEP 50K ";
    //
#ifdef __t0mm13b_defiant__
    printf("Handling Radio Reset!\n");
#else
    logmsg("Handling Radio Reset");
#endif    
    handle_band(reset_stg1);
    handle_tunestep(reset_stg3);
    handle_autoseek(reset_stg2);
}

void handle_autoseek(const char *buf){
    int rv = 0, status = 0, len, dir = -1, freq = -1, cmdPrimOk = 0, ioctlparams[2];
    regmatch_t pmatch[3];
    ssize_t nmatch = 2;
    char *sCmdToken = NULL, *sAutoSeekDir = NULL, *sAutoSeekFreq = NULL, *ptrBuf = NULL;
    if (regexec(&cmds[AUTOSEEK_IDX].cmdregexp, buf, nmatch, pmatch, 0) == 0){
        sCmdToken = strtok((char*)buf, delimiters);
        while (sCmdToken){
            if (!strcasecmp(sCmdToken, "AUTOSEEK")) cmdPrimOk++;
            else{
                // This must be the tunestep type....
                switch(cmdPrimOk){
                    case 1  :   // Direction handling...
                                len = strlen(sCmdToken);                
                                if (!(sAutoSeekDir = (char *)malloc((len + 1) * sizeof(char)))) panic("[%s:handle_autoseek(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                                strncpy(sAutoSeekDir, sCmdToken, len);
                                *(sAutoSeekDir + len) = '\0';
                                cmdPrimOk++;
                                break;
                    case 2  :   // Frequency ...
                                len = strlen(sCmdToken);                
                                if (!(sAutoSeekFreq = (char *)malloc((len + 1) * sizeof(char)))) panic("[%s:handle_autoseek(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                                strncpy(sAutoSeekFreq, sCmdToken, len);
                                *(sAutoSeekFreq + len) = '\0';
                                cmdPrimOk++;
                                break;
                }
            }
            sCmdToken = strtok(NULL, delimiters);
        }
        if (cmdPrimOk > 1){
            if (!strcasecmp(sAutoSeekDir, "DOWN")) dir = 0;
            if (!strcasecmp(sAutoSeekDir, "UP")) dir = 1;
            if (dir == -1){
#ifdef __t0mm13b_defiant__
                printf("Unknown direction type %s!\n", sAutoSeekDir);
#else
		        logmsg("Unknown direction type %s!\n", sAutoSeekDir);
#endif
                if (sAutoSeekDir) free(sAutoSeekDir);
                if (sAutoSeekFreq) free(sAutoSeekFreq);
                return; // END THIS IMMEDIATELY...
            }
            freq = atoi(sAutoSeekFreq);
            if (freq > -1){
                // just do it...
                ioctlparams[0] = dir;
                ioctlparams[1] = freq;
#ifdef __t0mm13b_defiant__
                printf("AutoSeek %s from %d\n", sAutoSeekDir, freq);
#else
                rv = ioctl(g_rdfmdev, FM_SEEK, ioctlparams);
#endif                
				fprintf(g_fpwrstatpipe, "[FM_SEEK] - rv = %d; next station: %d\n", rv, ioctlparams[1]);
            }
        }
        if (sAutoSeekDir) free(sAutoSeekDir);
        if (sAutoSeekFreq) free(sAutoSeekFreq);
    }
}

void handle_volume(const char *buf){
    int rv = 0, status = 0, len, vol, cmdPrimOk = 0;
    char *sCmdToken = NULL, *sVolume = NULL;
    regmatch_t pmatch[2];
    ssize_t nmatch = 2;
    if (regexec(&cmds[VOLUME_IDX].cmdregexp, buf, nmatch, pmatch, 0) == 0){
        sCmdToken = strtok((char*)buf, delimiters);
        while (sCmdToken){
            if (!strcasecmp(sCmdToken, "VOLUME")) cmdPrimOk++;
            else{
                len = strlen(sCmdToken);                
                if (!(sVolume = (char *)malloc((len + 1) * sizeof(char)))) 
                    panic("[%s:handle_volume(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                strncpy(sVolume, sCmdToken, len);
                *(sVolume + len) = '\0';
                cmdPrimOk++;
            }
            sCmdToken = strtok(NULL, delimiters);
        }
        if (cmdPrimOk == 1){
            // 
#ifdef __t0mm13b_defiant__
            printf("Handle Radio Volume!\n");
#else            
            rv = ioctl(fd, FM_GET_VOL, &status);
#endif            
			fprintf(g_fpwrstatpipe, "[FM_GET_VOL] - rv = %d; status = %d\n", rv, status);
        }else{
            if (cmdPrimOk > 1){
                vol = atoi(sVolume);
#ifdef __t0mm13b_defiant__
                printf("Handle Radio Volume @ %s converted to %d!\n", sVolume, vol);
#else
                rv = ioctl(fd, FM_SET_VOL, &vol);
#endif
                if (sVolume) free(sVolume);                        
                fprintf(g_fpwrstatpipe, "[FM_SET_VOL] - rv = %d\n", rv);
            }
        }
    }
}

void handle_tune(const char *buf){
    int rv = 0, status = 0, len, tune = 0, cmdPrimOk = 0;
    regmatch_t pmatch[3];
    ssize_t nmatch = 2;
    char *sCmdToken = NULL, *sTuneFreq = NULL, *ptrBuf = NULL;
    if (regexec(&cmds[TUNE_IDX].cmdregexp, buf, nmatch, pmatch, 0) == 0){
        // TUN figure
        /*if (!(ptrBuf = (char*)malloc((strlen(buf)+1) * sizeof(char)))) panic("[%s:handle_autoseek(...) @ %d] - malloc failed", __FILE__, __LINE__);
        strncpy(ptrBuf, buf, strlen(buf));
        *(ptrBuf + strlen(buf)) = '\0';*/
        sCmdToken = strtok((char*)buf, delimiters);
        while (sCmdToken){
            if (!strcasecmp(sCmdToken, "TUNE")) cmdPrimOk++;
            else{
                // This must be the frequency....
                len = strlen(sCmdToken);                
                if (!(sTuneFreq = (char *)malloc((len + 1) * sizeof(char)))) panic("[%s:handle_tune(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                strncpy(sTuneFreq, sCmdToken, len);
                *(sTuneFreq + len) = '\0';
                tune = atoi(sTuneFreq);
                cmdPrimOk++;
            }
            sCmdToken = strtok(NULL, delimiters);
        }
        if (cmdPrimOk > 1){
#ifdef __t0mm13b_defiant__
            printf("Handle Radio tune @ %s converted to %d!\n", sTuneFreq, tune);
#else
            rv = ioctl(g_rdfmdev, FM_TUNE, &tune);
#endif
            fprintf(g_fpwrstatpipe, "[FM_TUNE] - rv = %d\n", rv);
        }else{
#ifdef __t0mm13b_defiant__
            printf("Handle Radio Tune - requires parameter!!\n");
#else
            logmsg("Tune requires frequency...");
#endif            
        }
    }
    if (sTuneFreq) free(sTuneFreq);
}

void handle_band(const char *buf){
    int rv = 0, status = 0, len, band = -1, cmdPrimOk = 0;
    regmatch_t pmatch[3];
    ssize_t nmatch = 2;
    char *sCmdToken = NULL, *sBandType = NULL;
    if (regexec(&cmds[BAND_IDX].cmdregexp, buf, nmatch, pmatch, 0) == 0){
        sCmdToken = strtok((char*)buf, delimiters);
        while (sCmdToken){
            if (!strcasecmp(sCmdToken, "BAND")) cmdPrimOk++;
            else{
                // This must be the band type....
                len = strlen(sCmdToken);                
                if (!(sBandType = (char *)malloc((len + 1) * sizeof(char)))) panic("[%s:handle_band(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                strncpy(sBandType, sCmdToken, len);
                *(sBandType + len) = '\0';
                cmdPrimOk++;
            }
            sCmdToken = strtok(NULL, delimiters);
        }
        if (cmdPrimOk == 1){
            // QUERY!
#ifdef __t0mm13b_defiant__
            printf("Handle Radio Band type!\n");
#else
		    rv = ioctl(g_rdfmdev, FM_GET_BAND, &status);
#endif
            fprintf(g_fpwrstatpipe, "[FM_GET_BAND] - rv = %d; status = %d\n", rv, status);
            
        }else{
            if (!strcasecmp(sBandType, "GENERIC")) band = 0;
            if (!strcasecmp(sBandType, "JAPANW")) band = 1;
            if (!strcasecmp(sBandType, "JAPAN")) band = 2;
            if (!strcasecmp(sBandType, "RESERVED")) band = 3;
            if (band > -1){
                // just do it...
#ifdef __t0mm13b_defiant__
                printf("Setting band type to %s\n", sBandType);
#else
                rv = ioctl(g_rdfmdev, FM_SET_BAND, &band);
#endif                
				fprintf(g_fpwrstatpipe, "[FM_SET_BAND] - rv = %d\n", rv);
            }else{
#ifdef __t0mm13b_defiant__
                printf("Unknown band type - %s!\n", sBandType);
#else
                logmsg("FM_SET_BAND - unknown parameter %s!\n", sBandType);
#endif
                
            }
        }
        if (sBandType) free(sBandType);
    }
}

void handle_tunestep(const char *buf){
    int rv = 0, status = 0, len, tunestep = -1, cmdPrimOk = 0;
    regmatch_t pmatch[3];
    ssize_t nmatch = 2;
    char *sCmdToken = NULL, *sTuneStepType = NULL;
    if (regexec(&cmds[TUNESTEP_IDX].cmdregexp, buf, nmatch, pmatch, 0) == 0){
        sCmdToken = strtok((char*)buf, delimiters);
        while (sCmdToken){
            if (!strcasecmp(sCmdToken, "TUNESTEP")) cmdPrimOk++;
            else{
                // This must be the tunestep type....
                len = strlen(sCmdToken);                
                if (!(sTuneStepType = (char *)malloc((len + 1) * sizeof(char)))) panic("[%s:handle_tunestep(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                strncpy(sTuneStepType, sCmdToken, len);
                *(sTuneStepType + len) = '\0';
                cmdPrimOk++;
            }
            sCmdToken = strtok(NULL, delimiters);
        }
        if (cmdPrimOk == 1){
            // QUERY!
#ifdef __t0mm13b_defiant__
            printf("Handle Tune Stepper type!\n");
#else
		    rv = ioctl(g_rdfmdev, FM_GET_SPACE, &status);
#endif
            fprintf(g_fpwrstatpipe, "[FM_GET_SPACE] - rv = %d; status = %d\n", rv, status);
            
        }else{
            if (!strcasecmp(sTuneStepType, "50K")) tunestep = 0;
            if (!strcasecmp(sTuneStepType, "100K")) tunestep = 1;
            if (!strcasecmp(sTuneStepType, "200K")) tunestep = 2;
            if (tunestep > -1){
                // just do it...
#ifdef __t0mm13b_defiant__
                printf("Setting Tune Stepper to %s\n", sTuneStepType);
#else
                rv = ioctl(g_rdfmdev, FM_SET_SPACE, &tunestep);
#endif                
				fprintf(g_fpwrstatpipe, "[FM_SET_SPACE] - rv = %d\n", rv);
            }else{
#ifdef __t0mm13b_defiant__
                printf("Unknown tunestepper type - %s!\n", sTuneStepType);
#else
                logmsg("FM_SET_SPACE - unknown parameter %s!\n", sTuneStepType);
#endif
            }
        }
        if (sTuneStepType) free(sTuneStepType);
    }
}


void handle_audio(const char *buf){
    int rv = 0, status = 0, len, track = -1, cmdPrimOk = 0;
    regmatch_t pmatch[3];
    ssize_t nmatch = 2;
    char *sCmdToken = NULL, *sAudioType = NULL;
    if (regexec(&cmds[AUDIO_IDX].cmdregexp, buf, nmatch, pmatch, 0) == 0){
        sCmdToken = strtok((char*)buf, delimiters);
        while (sCmdToken){
            if (!strcasecmp(sCmdToken, "AUDIO")) cmdPrimOk++;
            else{
                // This must be the track type....
                len = strlen(sCmdToken);                
                if (!(sAudioType = (char *)malloc((len + 1) * sizeof(char)))) panic("[%s:handle_audio(...) @ %d] - malloc failed.", __FILE__, __LINE__);
                strncpy(sAudioType, sCmdToken, len);
                *(sAudioType + len) = '\0';
                cmdPrimOk++;
            }
            sCmdToken = strtok(NULL, delimiters);
        }
        if (cmdPrimOk == 1){
            // QUERY!
#ifdef __t0mm13b_defiant__
            printf("Handle Radio audio track type!\n");
#else
		    rv = ioctl(g_rdfmdev, FM_GET_AUDIOTRACK, &status);
#endif
            fprintf(g_fpwrstatpipe, "[FM_GET_AUDIOTRACK] - rv = %d; status = %d\n", rv, status);
            
        }else{
            if (!strcasecmp(sAudioType, "STEREO")) track = 0;
            if (!strcasecmp(sAudioType, "MONO")) track = 1;
            if (track > -1){
                // just do it...
#ifdef __t0mm13b_defiant__
                printf("Setting track type to %s\n", sAudioType);
#else
                rv = ioctl(g_rdfmdev, FM_SET_AUDIOTRACK, &track);
#endif                
				fprintf(g_fpwrstatpipe, "[FM_SET_AUDIOTRACK] - rv = %d\n", rv);
            }else{
#ifdef __t0mm13b_defiant__
                printf("Unknown audio track type - %s!\n", sAudioType);
#else
                logmsg("[FM_SET_AUDIOTRACK] - unknown parameter %s!\n", sAudioType);
#endif
                
            }
        }
        if (sAudioType) free(sAudioType);
    }
}

void handle_shutdown(const char *buf){
#ifdef __t0mm13b_defiant__
    printf("Shutting down...\n");
#else
    logmsg("Shutting down!\n");    
#endif
    g_keepRunning = 0;
}