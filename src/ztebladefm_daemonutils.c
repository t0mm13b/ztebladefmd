#include "ztebladefm.h"

extern int g_keepRunning;
static int g_exit_ip = 0;
int g_lockfp_ztebladefmd = 0;
static void signal_handler(int sig);
static void dumpstack(void);
static void child_handler(int signum);


/**
  * http://www-theorie.physik.unizh.ch/~dpotter/howto/daemonize
  */
void daemonize( const char *lockfile )
{
    pid_t pid, sid, parent;
    int lfp = -1;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Create the lock file as the current user */
    if ( lockfile && lockfile[0] ) {
        lfp = open(lockfile,O_RDWR|O_CREAT,0640);
        if ( lfp < 0 ) {
            logmsg("unable to create lock file %s, code=%d (%s)", lockfile, errno, strerror(errno) );     
            exit(EXIT_FAILURE);
        }
    }
#ifdef __t0mm13b_defiant__
    /* Drop user if there is one, and we were run as root */
    if ( getuid() == 0 || geteuid() == 0 ) {
        struct passwd *pw = getpwnam(RUN_AS_USER);
        if ( pw ) {
            logmsg("setting user to %s", RUN_AS_USER);
            setuid( pw->pw_uid );
        }
    }
#endif

    /* Trap signals that we expect to recieve */
    signal(SIGCHLD,child_handler);
    signal(SIGUSR1,child_handler);
    signal(SIGALRM,child_handler);

    //printf("Trapping signals within daemonize(...)\n");
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        logmsg("unable to fork daemon, code=%d (%s)", errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {

        /* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
           for two seconds to elapse (SIGALRM).  pause() should not return. */
        alarm(2);
        pause();

        exit(EXIT_FAILURE);
    }

    /* At this point we are executing as the child process */
    parent = getppid();

    /* Cancel certain signals */
    signal(SIGCHLD,SIG_DFL); /* A child process dies */
    signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
    signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */

    //printf("Cancelling certain signals....\n");
    /* Change the file mode mask */
    umask(0);

    //printf("Changed the file mode mask...\n");
    
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        logmsg("unable to create a new session, code %d (%s)", errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    //printf("Created a new sid...\n");
    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        //syslog( LOG_ERR, "unable to change directory to %s, code %d (%s)",
        //        "/", errno, strerror(errno) );
        logmsg("unable to change directory to /, code %d (%s)", errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    //printf("Changed dir to /\n");
    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);

    //logmsg("Redirected!");
    /* Tell the parent process that we are A-okay */
    kill( parent, SIGUSR1 );
    //logmsg("Attaboy!\n");
}
  
static void child_handler(int signum)
{
    switch(signum) {
    case SIGALRM: exit(EXIT_FAILURE); break;
    case SIGUSR1: exit(EXIT_SUCCESS); break;
    case SIGCHLD: exit(EXIT_FAILURE); break;
    }
}

void exithandler(void){
#if __t0mm13b_defiant__
    if (unlink(ZTEBLADEFM_PIPE_CMD) < 0){
        // handle condition here..
        //panic("[%s:tune_in(...) @ %d] - Could not unlink %s.", __FILE__, __LINE__, ZTEBLADEFM_PIPE_CMD);
    }else logmsg("Cleaned up command pipe!");
    if (unlink(ZTEBLADEFM_PIPE_STATUS) < 0){
        // handle condition here..
        //panic("[%s:tune_in(...) @ %d] - Could not unlink %s.", __FILE__, __LINE__, ZTEBLADEFM_PIPE_STATUS);
    }else logmsg("Cleaned up status pipe!");
#endif
    if (g_exit_ip) return;
    if (unlink(ZTEBLADEFM_LOCK) < 0) 
        logmsg("[%s:exithandler @ %d] - Could not unlink \'%s\'", __FILE__, __LINE__, ZTEBLADEFM_LOCK);

    sigemptyset(&g_sigact.sa_mask);
    logmsg("Terminated.");
    if (!g_exit_ip) g_exit_ip++;
}

void init_signals(void){
    g_sigact.sa_handler = signal_handler;
    sigemptyset(&g_sigact.sa_mask);
    g_sigact.sa_flags = 0;
    sigaction(SIGINT, &g_sigact, (struct sigaction *)NULL);
 
    sigaddset(&g_sigact.sa_mask, SIGSEGV);
    sigaction(SIGSEGV, &g_sigact, (struct sigaction *)NULL);
 
    sigaddset(&g_sigact.sa_mask, SIGBUS);
    sigaction(SIGBUS, &g_sigact, (struct sigaction *)NULL);
 
    sigaddset(&g_sigact.sa_mask, SIGQUIT);
    sigaction(SIGQUIT, &g_sigact, (struct sigaction *)NULL);
 
    sigaddset(&g_sigact.sa_mask, SIGHUP);
    sigaction(SIGHUP, &g_sigact, (struct sigaction *)NULL);
 
    sigaddset(&g_sigact.sa_mask, SIGKILL);
    sigaction(SIGKILL, &g_sigact, (struct sigaction *)NULL);
}

void lock_ourselves(void){
    char str[10];
    g_lockfp_ztebladefmd = open(ZTEBLADEFM_LOCK,O_RDWR|O_CREAT|O_EXCL, 0640);
	if (g_lockfp_ztebladefmd < 0) panic("[%s:lock_ourselves @ %d] - Can not open lock file \'%s\'\n", __FILE__, __LINE__, ZTEBLADEFM_LOCK); /* can not open */
	/* only first instance continues */

	sprintf(str,"%d\n",getpid());
	write(g_lockfp_ztebladefmd, str,strlen(str)); /* record pid to lockfile */
    logmsg("[%s] - PID file locked", ZTEBLADEFMD_NM);
}

void panic(const char *fmt, ...){
    char buf[PANICBUF_LEN];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(buf, fmt, argptr);
    va_end(argptr);
#ifdef __t0mm13b_defiant__ 
    fprintf(stderr, "%s\n", buf);
#else       
    __android_log_print(ANDROID_LOG_VERBOSE, ZTEBLADEFMD_NM, "panic: %s", buf);
#endif    
    exit(-1);
}

void logmsg(const char *fmt, ...){
    char buf[LOGMSG_LEN];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(buf, fmt, argptr);
    va_end(argptr);
#ifdef __t0mm13b_defiant__
    fprintf(stdout, "%s\n", buf);
#else
    __android_log_print(ANDROID_LOG_VERBOSE, ZTEBLADEFMD_NM, "%s", buf);
#endif    
}

static void signal_handler(int sig){
    if (sig == SIGHUP) g_keepRunning = 0;
    if (sig == SIGSEGV || sig == SIGBUS){
        dumpstack();
        panic("FATAL: %s Fault\n", (sig == SIGSEGV) ? "Segmentation" : ((sig == SIGBUS) ? "Bus" : "Unknown"));
    }
    if (sig == SIGQUIT) g_keepRunning = 0;
    if (sig == SIGKILL) g_keepRunning = 0;
    if (sig == SIGINT) ;
}

static void dumpstack(void){
    /* Got this routine from http://www.whitefang.com/unix/faq_toc.html
    ** Section 6.5. Modified to redirect to file to prevent clutter
    */
#ifdef __t0mm13b_defiant__    
    char dbx[160];
    sprintf(dbx, "echo -ne 'detach\n' | gdb --eval-command=where --pid=%d > %d.dump", getpid(), getpid());
    system(dbx);
#endif    
    return;
}