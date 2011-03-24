#include "ztebladefm.h"

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

void init_regexps(void){
    int n = 0, okCnt = 0;
    for (; n < CMDS_LEN; n++){
        if (regcomp(&cmds[n].cmdregexp, cmds[n].cmd, REG_ICASE|REG_EXTENDED) == 0){
            cmds[n].regexOk = 0;
            okCnt++;
        }
    }
    if (okCnt == CMDS_LEN){
#ifdef __t0mm13b_defiant__
        printf("[Regexps OK!]\n");
#else
        fprintf(g_fpwrstatpipe, "[regexps OK!]\n");
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
        regfree(&cmds[n].cmdregexp);
        cmds[n].regexOk = -1;
    }
}

void parse_cmd(const char *buf){
    int n = 0;
    int matched = -1;
    for (; n < CMDS_LEN; n++){
        if (regexec(&cmds[n].cmdregexp, buf, 0, NULL, 0) == 0){
            matched = n;
            break;
        }
    }
    if (matched > -1 && cmds[matched].fpCommandHandler != NULL) (cmds[matched].fpCommandHandler)(buf);
}