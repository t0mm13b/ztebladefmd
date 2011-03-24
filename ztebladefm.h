#ifndef __ZTEBLADEFM_H__
#define __ZTEBLADEFM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <regex.h>
#include <syslog.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifndef __t0mm13b_defiant__
#include <android/log.h>
#include "fm_si4708.h"
#endif

// define's
#define EXIT_SUCCESS            0
#define EXIT_FAILURE            1
/* Change this to the user under which to run */
#define RUN_AS_USER             "t0mm13b"
#define DAEMON_NAME             "ztebladefmd"
//
#define LOGMSG_LEN              150
#define PANICBUF_LEN            256
#define CMDBUF_LEN              100
#ifdef __t0mm13b_defiant__
#define ZTEBLADEFM_LOCK         "/tmp/ztebladefm.lck"
#else
#define ZTEBLADEFM_LOCK         "/system/lost+found/ztebladefm.lck"
#endif
#define ZTEBLADEFMD_NM          "ztebladefmd"
#ifdef __t0mm13b_defiant__
#define ZTEBLADEFM_PIPE_CMD     "/tmp/ztebladefmCmds"
#define ZTEBLADEFM_PIPE_STATUS  "/tmp/ztebladefmStats"    
#else
#define ZTEBLADEFM_PIPE_CMD     "/system/lost+found/ztebladefmCmds"
#define ZTEBLADEFM_PIPE_STATUS  "/system/lost+found/ztebladefmStats" 
#endif
#define FM_DEV	                "/dev/si4708"
#define CMDS_LEN    12

// vars
typedef void (*fp_ptr)(const char *);
//
int g_rdcmdpipe, g_wrstatpipe, g_rdfmdev;
struct sigaction g_sigact;
FILE *g_fprdcmdpipe, *g_fpwrstatpipe;
//
struct Commands{
    const char *cmd;
    regex_t cmdregexp;
    int regexOk;
    fp_ptr fpCommandHandler;
};

#ifdef __cplusplus
extern "C" {
#endif

// ztebladefm_daemonutils.c
void daemonize(const char *);
void init_signals(void);
void lock_ourselves(void);
void exithandler(void);
void logmsg(const char *, ...);
void panic(const char *, ...);

// ztebladefm_regexps.c
void init_regexps(void);
void cleanup_regexps(void);

// ztebladefm_cmds.c
extern void handle_on(const char *);
extern void handle_off(const char *);
extern void handle_frequency(const char *);
extern void handle_reset(const char *);
extern void handle_shutdown(const char *);
extern void handle_tune(const char *);
extern void handle_volume(const char *);
extern void handle_band(const char *);
extern void handle_tunestep(const char *);
extern void handle_audio(const char *);
extern void handle_autoseek(const char *);



#ifdef __cplusplus
}
#endif


#endif /* __ZTEBLADEFM_H__ */