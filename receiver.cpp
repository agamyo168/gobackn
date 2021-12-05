#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "protocol.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

using namespace std;
// Global Variables
unsigned int server_socket;
bool network_layer_enabled = false;
bool time_out_f = false;
char j = 0;
packet networkLayerBuffer[8];
packet ackFrame[8] ={
    {'g','e','t',' ','f','r','m','1',},
    {'g','e','t',' ','f','r','m','2',},
    {'g','e','t',' ','f','r','m','3',},
    {'g','e','t',' ','f','r','m','4',},
    {'g','e','t',' ','f','r','m','5',},
    {'g','e','t',' ','f','r','m','6',},
    {'g','e','t',' ','f','r','m','7',},
    {'g','e','t',' ','f','r','m','8',},

};

// Functions

void startClient()
{
    

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS);
    address.sin_port = htons(PORT);

    while (connect(server_socket, (struct sockaddr *)&address, sizeof(address)))
        ; //Busy wait for server
    printf("Client connected\n");
}

void to_network_layer(packet *p)
{    
    for (char i = 0; i < 8; i++)
       networkLayerBuffer[j].data[i] = p->data[i] ;

    j = (j + 1) % 8;
}

void from_network_layer(packet *p)
{    
    for (char i = 0; i < 8; i++)
      p->data[i] = ackFrame[j].data[i];

    j = (j + 1) % 8;
}
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
    /*Return true if a <= b < c circularly; false otherwise.*/
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return true;
    else
        return false;
}


void from_physical_layer(frame *r)
{
    read(server_socket, r, sizeof(frame));
    printf("Frame Received\n");
    fflush(stdout);

    printf("seq = %d   ack = %d\n", r->seq, r->ack);
    fflush(stdout);

    printf("Info is : ");
    fflush(stdout);

    for (int i = 0; i < 8; i++)
        cout<< r->info.data[i];
    fflush(stdout);

    printf("\n");
    fflush(stdout);

    printf("*****************************************\n");
    fflush(stdout);
}

void wait_for_event(event_type *event)
{
    if (network_layer_enabled)
    {
        *event = network_layer_ready;
    }
    else if (time_out_f)
    {
        *event = timeout;
    }
    else
    {
        *event = frame_arrival;
    }
}
void enable_network_layer()
{
    network_layer_enabled = true;
}
void disable_network_layer()
{
    network_layer_enabled = false;
}
static void send_data(seq_nr frame_nr, seq_nr frame_expected, packet buffer[])
{
    /*Construct and send a data frame. */
    frame s;                                            /* scratch variable */
    s.info = buffer[frame_nr];                          /* insert packet into frame */
    s.seq = frame_nr;                                   /* insert sequence number into frame */
    s.ack = frame_expected; /* piggyback ack */
    to_physical_layer(&s);                              /* transmit the frame */
    //start_timer(frame_nr);                              /* start the timer running */
}

void to_physical_layer(frame *s)
{
    write(server_socket, s, sizeof(frame));
    printf("Frame Sent\n");
    fflush(stdout);

    printf("seq = %d   ack = %d\n", s->seq, s->ack);
    fflush(stdout);

    printf("Info is : ");
    fflush(stdout);

    for (int i = 0; i < 8; i++)
        cout<< s->info.data[i];
    fflush(stdout);

    printf("\n");
    fflush(stdout);

    printf("*****************************************\n");
    fflush(stdout);
}

int main()
{   //Initialize network buffer
    seq_nr next_frame_to_send;  /* MAX SEQ > 1; used for outbound stream */
    seq_nr ack_expected;        /* oldest frame as yet unacknowledged */
    seq_nr frame_expected;      /* next frame expected on inbound stream */
    frame r;                    /* scratch variable */
    packet buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
    seq_nr nbuffered;           /* number of output buffers currently in use */
    seq_nr i;                   /* used to index into the buffer array */
    event_type event;
    //enable_network_layer(); /* allow network layer ready events */
    ack_expected = 0;       /* next ack expected inbound */
    next_frame_to_send = 0; /* next frame going out */
    frame_expected = 0;     /* number of frame expected inbound */
    nbuffered = 4;          /* initially no packets are buffered */

    startClient();
    while(true)
    {   wait_for_event(&event);
        if(event == network_layer_ready)
        {
        from_network_layer(&buffer[next_frame_to_send]);  
        nbuffered = nbuffered + 1;                             /* expand the sender's window */
        send_data(next_frame_to_send, frame_expected, buffer); /* transmit the frame */
        inc(next_frame_to_send);                               /* advance sender's upper window edge */

        }
        else if(event == frame_arrival)
            {
            from_physical_layer(&r); /* get incoming frame from physical layer */
            if (r.seq == frame_expected)
            {
                /*Frames are accepted only in order. */
                to_network_layer(&r.info); /* pass packet to network layer */
                inc(frame_expected);       /* advance lower edge of receiver's window */

            }
            while (between(ack_expected, r.ack, next_frame_to_send))
            {
                /*Handle piggybacked ack. */
                nbuffered = nbuffered - 1; /* one frame fewer buffered */
                //stop_timer(ack_expected);  /* frame arrived intact; stop timer */
                inc(ack_expected); /* contract sender's window */
            }
            }
        if (nbuffered < MAX_SEQ)
            enable_network_layer();
        else
            disable_network_layer();
     

    }
    return 0;
}