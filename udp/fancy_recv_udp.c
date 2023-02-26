#include	<signal.h>
#include	<errno.h>
#include	<strings.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include        <arpa/inet.h>
#include        <sys/time.h>

int
main()
{
    int	socket_fd, cc, fsize, hits;
    fd_set mask;
    struct timeval timeout;
    struct sockaddr_in	s_in, from;
    void printsin();

    struct {
	char	head;
	u_long	body;
	char	tail;
    } msg;

    socket_fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
	perror ("recv_udp:socket");
	exit (1);
    }

    bzero((char *) &s_in, sizeof(s_in));  /* They say you must do this    */
    s_in.sin_family = (short) AF_INET;
    s_in.sin_addr.s_addr = htonl(INADDR_ANY);    /* WILDCARD */
    s_in.sin_port = htons((u_short)0x3333);
    printsin( &s_in, "RECV_UDP", "Local socket is:"); fflush(stdout);
/*
   bind port 0x3333 on the current host to the socket accessed through
   socket_fd. If port in use, die.
*/
    if (bind(socket_fd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0) {
	    perror("recv_udp:bind");
	    exit(1);
	}

    for(;;) {
	fsize = sizeof(from);
/* Here's the new stuff. Hang a select on the file descriptors 0 (stdin)
   and socket_fd looking to see if either descriptor is able to be read.
   If it's stdin, shut down. If it's socket_fd, proceed as normal. If
   Nothing happens for a minute, shut down also.
*/
	FD_ZERO(&mask);
	FD_SET(0,&mask);
	FD_SET(socket_fd,&mask);
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;
        if ((hits = select(socket_fd+1, &mask, (fd_set *)0, (fd_set *)0,
                           &timeout)) < 0) {
	  perror("recv_udp:select");
	  exit(1);
	}
        if ( (hits==0) || ((hits>0) && (FD_ISSET(0,&mask))) ) {
	  printf("Shutting down\n");
	  exit(0);
	}
	cc = recvfrom(socket_fd,&msg,sizeof(msg),0,(struct sockaddr *)&from, (socklen_t *)&fsize);
	if (cc < 0)
	    perror("recv_udp:recvfrom");
	printsin( &from, "recv_udp: ", "Packet from:");
	printf("Got data ::%c%d%c\n",msg.head,ntohl(msg.body),msg.tail);
	fflush(stdout);
    }
    return(0);
}

void
printsin( sin, m1, m2 )
struct sockaddr_in *sin;
char *m1, *m2;
{

    printf ("%s %s:\n", m1, m2);
    printf ("  family %d, addr %s, port %d\n", sin -> sin_family,
	    inet_ntoa(sin -> sin_addr), ntohs((unsigned short)(sin -> sin_port)));
}


