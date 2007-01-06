
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#ifdef MAX
#undef MAX
#endif

#define MAX(x,y) ((x) > (y) ? (x) : (y))

enum {
	NUM_READ_FD         =   128,
	NUM_CALLBACKS       =   128,
};

typedef struct {
	int fd;
	void (*callback)(int fd);
} read_fd;

typedef struct {
	int timeout;           // timeout in milliseconds
	struct timeval starttime;
	void (*callback)(void);
} timeout_callback;

typedef void (* void_callback_t )( void );

static read_fd r_list[NUM_READ_FD];
static int num_read_fd = 0;
static fd_set master_fds;
static int fdmax = 0;

static timeout_callback to_callback = {0};

static void_callback_t callback_list[NUM_CALLBACKS];
static int num_callbacks = 0;

static int elapsed_time(struct timeval *starttime)
{
   	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int)(((tv.tv_sec - starttime->tv_sec) * 1000) +
			((tv.tv_usec - starttime->tv_usec) / 1000));
}

static void call_timeout_callback( void )
{
	void (*callback)( void ) = to_callback.callback;

	to_callback.callback = NULL;

	callback();
}


void interrupt_add_read_fd(int fd, void (*callback)(int) )
{
	int i;
	int flags;
	if( num_read_fd == 0 ) {
		FD_ZERO(&master_fds);
	} else if( num_read_fd >= NUM_READ_FD ) {
		fprintf(stderr, "not enough storage for read file descriptor\n");
		fprintf(stderr, "increase NUM_READ_FD in interrupt.c\n");
		exit(1);
	}

#if defined(O_NONBLOCK)
	/* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
	if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
		flags = 0;
	}
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	/* Otherwise, use the old way of doing it */
	flags = 1;
	ioctl(fd, FIOBIO, &flags);
#endif
	for (i=0; i<NUM_READ_FD; i++) {
		if (r_list[i].fd == -1) {
			r_list[i].fd = fd;
			r_list[i].callback = callback;
			num_read_fd++;
			fdmax = MAX(fdmax, fd);
			break;
		}
	}
	FD_SET(fd, &master_fds);
}

void interrupt_remove_read_fd(int fd)
{
	int i;
	for (i=0; i<NUM_READ_FD; i++) {
		if (r_list[i].fd == fd) {
			r_list[i].fd = -1;
			--num_read_fd;
		}
	}
	FD_CLR(fd,&master_fds);
}

int interrupt_get_elapsed_time( void )
{
	if(to_callback.callback == NULL) {
		return 0;
	}
	return elapsed_time(&(to_callback.starttime));
}

void interrupt_set_timeout(int timeout, void (*callback)(void) )
{
	to_callback.timeout = timeout;
	to_callback.callback = callback;
	gettimeofday(&(to_callback.starttime), NULL);
}

void interrupt_add_callbacks( void (*callback)(void) )
{
	callback_list[num_callbacks] = callback;
	num_callbacks++;
}

void interrupt_loop( void )
{
	int curr_sock_fd;
	fd_set read_fds = master_fds;
	struct timeval to = {0};
	int i;

	int ret;

	// Calling callbacks
	for(i = 0; i < num_callbacks; i++) {
		callback_list[i]();
	}
	num_callbacks = 0;


	if(to_callback.callback != NULL) {
		int to_int = to_callback.timeout - elapsed_time(&(to_callback.starttime));
		if( to_int > 0 ) {
			to.tv_sec = to_int / 1000;
			to.tv_usec = (to_int % 1000) * 1000;
		} else {
			call_timeout_callback();
			return;
		}
	}

	//printf("fdmax = %d\n", fdmax);
	if( to.tv_sec == 0 && to.tv_usec == 0 ) {
		ret = select(fdmax+1, &read_fds, NULL, NULL, NULL );
	} else {
		ret = select(fdmax+1, &read_fds, NULL, NULL, &to );
	}

	if( ret == -1 ) {
		perror("select");
		exit(1);
	} else if( ret == 0 ) {
		call_timeout_callback();
	} else {
		// adjust timeout value
		for(curr_sock_fd = 0; curr_sock_fd <= fdmax; curr_sock_fd++) {
			if (FD_ISSET(curr_sock_fd, &read_fds)) {
				int i;
				for( i = 0; i < num_read_fd; i++ ) {
					if(curr_sock_fd == r_list[i].fd ) {
						(r_list[i].callback)(curr_sock_fd);
						break;
					}
				}
			}
		}
	}
}

void interrupt_init()
{
	int i;
	for (i=0; i<NUM_READ_FD; i++) {
		r_list[i].fd = -1;
	}
}

// ======================
// Test cases
// ----------------------
#if 0
void timeout()
{
	printf("timeout occur\n");
}

void from_user(int fd)
{
	char buf[1024];
	int num_bytes;
	int i = 0;
	num_bytes = read(0, buf, 1024);

	printf("from user: \n");
	for( i = 0; i < num_bytes; i++ ) {
		printf("%x ", buf[i]);
	}
	printf("\n");
}

int main()
{
	interrupt_add_read_fd(0, from_user);
	interrupt_set_timeout(5000, timeout);

	while( 1 ) {
		interrupt_loop();
	}
}

#endif

