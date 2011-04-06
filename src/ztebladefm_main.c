#include "ztebladefm.h"

int g_keepRunning = 1;
static fd_set g_fdset_r;
static fd_set g_fdset_w;
//
static void tune_in(void);
static void check_select(void);
static void init_open_commandpipe(void);
static void init_open_statuspipe(void);
static int fd_set_blocking(int, int);
static int fifo_exists(const char *fifoname);

int main(int argc, char **argv){
    //
    atexit(exithandler);
    //
    daemonize(ZTEBLADEFM_LOCK);
    logmsg("[%s:main()] entering daemon mode...", __FILE__);  
    tune_in();
    logmsg("[%s:main()] leaving daemon mode...", __FILE__);
    //
    return 0;
}

static int fifo_exists(const char *fifoname){
	struct stat filestat;
	if (!stat(fifoname, &filestat)){
		if (S_ISFIFO(filestat.st_mode)){
			return 0;
		}
		return 1;
	}
	return 1;
}

static void tune_in(void){
    int selfd;
    //logmsg("[%s: tune_in(...)] *** ENTER ***", __FILE__);
    init_signals();
    //logmsg("[%s: tune_in(...)] Signals initialized", __FILE__);
    //lock_ourselves();
    init_regexps();
    //logmsg("[%s: tune_in(...)] Regexps initialized", __FILE__);
    // BEGIN HERE
	if (fifo_exists(ZTEBLADEFM_PIPE_CMD)){
		int rv = mkfifo(ZTEBLADEFM_PIPE_CMD, 0666);
		if (rv < 0){
			// handle condition here..
			panic("[%s:tune_in(...) @ %d] - Could not mkfifo %s.", __FILE__, __LINE__, ZTEBLADEFM_PIPE_CMD);
		}
	}
	if (fifo_exists(ZTEBLADEFM_PIPE_STATUS)){
		int rv = mkfifo(ZTEBLADEFM_PIPE_STATUS, 0666);
		if (rv < 0){
			// handle condition here..
			panic("[%s:tune_in(...) @ %d] - Could not mkfifo %s.", __FILE__, __LINE__, ZTEBLADEFM_PIPE_STATUS);
		}
	}
#if __t0mm13b_defiant__
    int rv = mkfifo(ZTEBLADEFM_PIPE_CMD, 0666);
    if (rv < 0){
        // handle condition here..
        panic("[%s:tune_in(...) @ %d] - Could not mkfifo %s.", __FILE__, __LINE__, ZTEBLADEFM_PIPE_CMD);
    }
    rv = mkfifo(ZTEBLADEFM_PIPE_STATUS, 0666);
    if (rv < 0){
        // handle condition here..
        panic("[%s:tune_in(...) @ %d] - Could not mkfifo %s.", __FILE__, __LINE__, ZTEBLADEFM_PIPE_STATUS);
    }
#else    
    g_rdfmdev = open(FM_DEV, O_RDONLY);
#endif
    //
    init_open_statuspipe();
    init_open_commandpipe();
    // logic routine from here ... http://stackoverflow.com/questions/1735781/non-blocking-pipe-using-popen
    while (g_keepRunning){
        selfd = select(FD_SETSIZE, &g_fdset_r, (fd_set*)0, (fd_set*)0,NULL);
        if (selfd < 0) panic("Could not select...");
        if (selfd > 0){
            check_select();
        }
    }
    if (g_fprdcmdpipe != NULL) fclose(g_fprdcmdpipe);
    if (g_fpwrstatpipe != NULL) fclose(g_fpwrstatpipe);
#ifdef __t0mm13b_defiant__
    ;
#else    
    close(g_rdfmdev);
#endif
    cleanup_regexps();
    // END HERE
}
static void init_open_statuspipe(void){
    int nPipeFlags = 0;
    if (!(g_fpwrstatpipe = fopen(ZTEBLADEFM_PIPE_STATUS,"w")))
        panic("[%s:init_open_statuspipe(...) @ %d] - Could not open %s for writing!", __FILE__, __LINE__, ZTEBLADEFM_PIPE_STATUS);
    g_wrstatpipe = fileno(g_fpwrstatpipe);
    if (!fd_set_blocking(g_wrstatpipe, 0)){
        panic("[%s:init_open_statuspipe(...) @ %d] - Could not set status pipe to non-block!", __FILE__, __LINE__);
    }else{
        logmsg("Set status pipe to non-block OK!\n");
    }
    if (setvbuf(g_fpwrstatpipe, NULL, _IONBF, CMDBUF_LEN) != 0)
        panic("[%s:init_open_statuspipe(...) @ %d] - Could not set buffer for status pipe!", __FILE__, __LINE__);
    FD_ZERO(&g_fdset_w);
    FD_SET(g_wrstatpipe, &g_fdset_w);
}

//http://code.activestate.com/recipes/577384-setting-a-file-descriptor-to-blocking-or-non-block/
static int fd_set_blocking(int fd, int blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}
static void init_open_commandpipe(void){
    if (!(g_fprdcmdpipe = fopen(ZTEBLADEFM_PIPE_CMD, "r")))
        panic("[%s:init_open_commandpipe(...) @ %d] - Could not open %s for read!", __FILE__, __LINE__, ZTEBLADEFM_PIPE_CMD);
    g_rdcmdpipe = fileno(g_fprdcmdpipe);
    if (!fd_set_blocking(g_rdcmdpipe, 0)){
        panic("[%s:init_open_commandpipe(...) @ %d] - Could not set command pipe to non-block!", __FILE__, __LINE__);
    }else{
        logmsg("Set command pipe to non-block OK!\n");
    }
    if (setvbuf(g_fprdcmdpipe, NULL, _IONBF, CMDBUF_LEN) != 0) 
        panic("[%s:init_open_commandpipe(...) @ %d] - Could not set buffer for command pipe!", __FILE__, __LINE__);
    FD_ZERO(&g_fdset_r);
    FD_SET(g_rdcmdpipe, &g_fdset_r);
}

static void check_select(void){
    char buf[CMDBUF_LEN];
    char *bufptr = NULL;
    ssize_t rdcount;
    buf[0] = '\0';
    if (FD_ISSET(g_rdcmdpipe, &g_fdset_r)){
        rdcount = read(g_rdcmdpipe, buf, CMDBUF_LEN - 1);
        if (rdcount == -1 && errno == EAGAIN){
            // No data...
#ifdef _t0mm13b_defiant_
            printf("No data\n");
#endif                        
        }else{
            if (rdcount > 0){
                // now handle the contents of buffer
                buf[rdcount - 1] = '\0';
                if (!(bufptr = (char *)malloc(rdcount * sizeof(char)))) 
                    panic("[%s:check_select(...) @ %d] - malloc failed!", __FILE__, __LINE__);
                strncpy(bufptr, &buf[0], rdcount);
                *(bufptr + rdcount) = '\0';
                printf("YO DAWG - \"%s\"\n", bufptr);
                parse_cmd(bufptr);
                if (bufptr) free(bufptr);
                //fflush(g_fprdcmdpipe);
            }else{
                // pipe closed
#ifdef __t0mm13b_defiant__
                printf("Pipe Closed!\n");
                //
#endif                
                if (g_fprdcmdpipe != NULL) fclose(g_fprdcmdpipe);
                //
                init_open_commandpipe();
            }          
        }
    }
}
