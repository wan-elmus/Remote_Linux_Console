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



int
main()
{
    int	socket_fd, cc, fsize;
    struct sockaddr_in	s_in, from;
    void printsin();    

    struct {
	char	head;
	u_long	body;
	char	tail;
    } msg;

/*
   Create the socket to be used for datagram reception. Initially,
   it has no name in the internet (or any other) domain.
*/
    socket_fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
	perror ("recv_udp:socket");
	exit (1);
    }

/*
   In order to attach a name to the socket created above, first fill
   in the appropriate blanks in an inet socket address data structure
   called "s_in". We blindly pick port number 0x3333. The second step
   is to BIND the address to the socket. If port 0x3333 is in use, the
   bind system call will fail detectably.
*/

    bzero((char *) &s_in, sizeof(s_in));  /* They say you must do this    */

    s_in.sin_family = (short)AF_INET;
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
	cc = recvfrom(socket_fd,&msg,sizeof(msg),0,(struct sockaddr *)&from,&fsize);
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

