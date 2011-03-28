#include "ztebladefm.h"

#ifdef __USE_POSIX_REGEXP__
struct Commands cmds[] = {
    {"^ON$", NULL, -1, NULL}, // 0
    {"^OFF$", NULL, -1, NULL}, // 1
    {"^TUNE([[:space:]]{0,})([[:digit:]]{1,6})$", NULL, -1, NULL}, // 2 DONE!
    {"^VOLUME(([[:space:]]{0,})([[:digit:]]{1,2})){0,1}$", NULL, -1, NULL},  // 3 DONE!
    {"^AUDIO([[:space:]]{0,})(MONO|STEREO)$", NULL, -1, NULL}, //4 DONE
    {"^BAND([[:space:]]{0,})(GENERIC|JAPANW|JAPAN|RESERVED)$", NULL, -1, NULL},  //5 DDONE!
    {"^TUNESTEP([[:space:]]{0,})(50K|100K|200K)$", NULL, -1, NULL}, //6 DONE
    {"^AUTOSEEK([[:space:]]{0,})(UP|DOWN)([[:space:]]{0,})([[:digit:]]{1,6})$", NULL, -1, NULL}, //7
    {"^FREQUENCY$", NULL, -1, NULL}, // 8
    {"^RESET$", NULL, -1, NULL},  //9
    {"^SHUTDOWN$", NULL, -1, NULL}, //10
    {"^NOP$", NULL, -1, NULL}, //11
    };
#endif

#ifdef __USE_PCRE_REGEXP__
struct Commands cmds[] = {
    {"^ON$", NULL, -1, NULL}, // 0
    {"^OFF$", NULL, -1, NULL}, // 1
    {"^TUNE\\s+(\\d{1,6})$", NULL, -1, NULL}, // 2 DONE!
    {"^VOLUME(\\s+(\\d{1,2}))?$", NULL, -1, NULL},  // 3 DONE!
    {"^AUDIO(\\s+(MONO|STEREO))?$", NULL, -1, NULL}, //4 DONE
    {"^BAND(\\s+(GENERIC|JAPANW|JAPAN|RESERVED))?$", NULL, -1, NULL},  //5 DDONE!
    {"^TUNESTEP(\\s+(50K|100K|200K))?$", NULL, -1, NULL}, //6 DONE
    {"^AUTOSEEK\\s+(UP|DOWN)\\s+(\\d{1,6})$", NULL, -1, NULL}, //7
    {"^FREQUENCY$", NULL, -1, NULL}, // 8
    {"^RESET$", NULL, -1, NULL},  //9
    {"^SHUTDOWN$", NULL, -1, NULL}, //10
    {"^NOP$", NULL, -1, NULL}, //11
    };
#endif

void init_regexps(void){
    int n = 0, okCnt = 0;
#ifdef __USE_PCRE_REGEXP__
    const char *error;
    int erroroffset;
    int offsetcount;
    int offsets[(2+1)*3]; // (max_capturing_groups+1)*3
#endif
    for (n = 0; n < CMDS_LEN; n++){
#ifdef __USE_POSIX_REGEXP__        
        if (regcomp(&cmds[n].cmdregexp, cmds[n].cmd, REG_ICASE|REG_EXTENDED) == 0){
#endif            
#ifdef __USE_PCRE_REGEXP__
        if ((cmds[n].cmdregexp = pcre_compile(cmds[n].cmd, PCRE_CASELESS | PCRE_EXTENDED, &error, &erroroffset, NULL)) != NULL){
#endif             
            //logmsg("[%s: init_regexps(...)] - (%d) - \"%s\"", __FILE__, n, cmds[n].cmd);
            cmds[n].regexOk = 0;
            okCnt++;
        }
    }
    if (okCnt == CMDS_LEN){
#ifdef __t0mm13b_defiant__
        printf("[Regexps OK!]\n");
#else
        logmsg("[%s: init_regexps(...)] - regexps OK!");
#endif        
    }else{
        panic("[%s:init_regexps(...) @ %d] - Regexps did not get compiled - only %d out of %d failed!", __FILE__, __LINE__, okCnt, CMDS_LEN);
    }
    cmds[0].fpCommandHandler = &handle_on;
    cmds[1].fpCommandHandler = &handle_off;
    cmds[2].fpCommandHandler = &handle_tune;
    cmds[3].fpCommandHandler = &handle_volume;
    cmds[4].fpCommandHandler = &handle_audio;
    cmds[5].fpCommandHandler = &handle_band;
    cmds[6].fpCommandHandler = &handle_tunestep;
    cmds[7].fpCommandHandler = &handle_autoseek;
    cmds[8].fpCommandHandler = &handle_frequency;
    cmds[9].fpCommandHandler = &handle_reset;
    cmds[10].fpCommandHandler = &handle_shutdown;    
}

void cleanup_regexps(void){
    int n = 0;
    for (; n < CMDS_LEN; n++){
#ifdef __USE_POSIX_REGEXP__        
        regfree(&cmds[n].cmdregexp);
#endif
#ifdef __USE_PCRE_REGEXP__
        pcre_free(cmds[n].cmdregexp);
#endif                
        cmds[n].regexOk = -1;
    }
}

void parse_cmd(const char *buf){
    int n = 0;
    int matched = -1;
#ifdef __USE_PCRE_REGEXP__
    const char *error;
    int erroroffset;
    int offsetcount;
    int offsets[(2+1)*3]; // (max_capturing_groups+1)*3
#endif    
    for (; n < CMDS_LEN; n++){
#ifdef __USE_POSIX_REGEXP__        
        if (regexec(&cmds[n].cmdregexp, buf, 0, NULL, 0) == 0){
#endif
#ifdef __USE_PCRE_REGEXP__
        if (pcre_exec(cmds[n].cmdregexp, NULL, buf, strlen(buf), 0, 0, offsets, (2+1)*3) > 0){
#endif            
            matched = n;
            break;
        }
    }
    if (matched > -1 && cmds[matched].fpCommandHandler != NULL) (cmds[matched].fpCommandHandler)(buf);
}