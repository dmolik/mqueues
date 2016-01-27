#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string.h>
#include <errno.h>

#include <pthread.h>

#include <mqueue.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1024

typedef struct _data {
	long  id;
	int   msg_size;
	char *mq_name;
} _data;

void *thread_worker(void *data)
{
	_data *d = (_data *) data;
	fprintf(stdout, "[%ld] entered thread\n", d->id);
	int  mq_fd;
	if ((mq_fd = mq_open(d->mq_name, O_RDWR|O_NONBLOCK)) == -1) {
		fprintf(stderr, "[%ld] failed to open mq: %s, %s\n", d->id, d->mq_name, strerror(errno));
		return NULL;
	}

	char *msg;
	      msg = malloc(d->msg_size);
	if (msg == NULL)
		fprintf(stderr, "[%ld] failed to allocate mem for sending message\n", d->id);

	snprintf(msg, d->msg_size, "[%ld] test\n", d->id);
	if (mq_send(mq_fd, msg, strlen(msg) + 1, 1) != 0)
		fprintf(stderr, "[%ld] failed to send message %s\n", d->id, strerror(errno));
	free(msg);
	if (mq_close(mq_fd) == -1) {
		fprintf(stderr, "[%ld] failed to close mq: %s, %s\n", d->id, d->mq_name, strerror(errno));
		return NULL;
	}
	free(d);
	return NULL;
}

int main (void)
{
	int  mq_fd;
	char *mq_name = "/test-mq";

	struct mq_attr attr;
	attr.mq_flags   = O_NONBLOCK;
	attr.mq_maxmsg  = 10; /* the default queue size on Linux */
	attr.mq_msgsize = 32;
	attr.mq_curmsgs = 0;

	mode_t omask;
	omask = umask(0);
	if ((mq_fd = mq_open(mq_name, O_RDWR|O_CREAT|O_NONBLOCK, (S_IRWXU | S_IRWXG), &attr)) == -1) {
		fprintf(stderr, "failed to open mq: %s, %s\n", mq_name, strerror(errno));
		return 1;
	}
	umask(omask);

	long t;
	pthread_t threads[4];
	for ( t = 0; t < 4; t++ ) {
		_data *d;
		       d = malloc(sizeof(_data));
		if (d == NULL)
			fprintf(stderr, "[master] failed to allocate mem for thread [%ld]\n", t);

		d->id       = t;
		d->mq_name  = mq_name;
		d->msg_size = attr.mq_msgsize;

		if (pthread_create(&threads[t], NULL, thread_worker, (void *)d) != 0) {
			fprintf(stderr, "failed to create thread: %ld, %s", t, strerror(errno));
			return 1;
		}
	}

	struct epoll_event ev, events[MAX_EVENTS];
	int nfds, epollfd;

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = mq_fd;
	if ((epollfd = epoll_create1(0)) == -1)
		fprintf(stderr, "epoll_create1");
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, mq_fd, &ev) == -1)
		fprintf(stderr, "epoll_ctl sfd");

	int   n = 0;
	int   num_received = 0;
	char *msg;
		  msg = malloc(attr.mq_msgsize);
	if (msg == NULL)
		fprintf(stderr, "[master] failed to allocate mem for receiving messages\n");
	for (;;) {
		if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1)
			fprintf(stderr, "epoll_wait");

		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == mq_fd) {
				if (mq_receive(mq_fd, msg, attr.mq_msgsize, NULL) == -1)
					fprintf(stderr, "[master] msg reveive error: %s\n", strerror(errno));
				num_received++;
				fprintf(stderr, "[master] recieved msg: %s\n", msg);
				bzero(msg, attr.mq_msgsize);
				if (num_received >= t) {
					free(msg);
					if (mq_close(mq_fd) == -1) {
						fprintf(stderr, "failed to close mq: %s, %s\n", mq_name, strerror(errno));
						return 1;
					}
					if (mq_unlink(mq_name) == -1) {
						fprintf(stderr, "failed to unlink mq: %s, %s\n", mq_name, strerror(errno));
						return 1;
					}
					pthread_exit(NULL);
					return 0;
				}
			}
		}
	}

	return 0;
}
