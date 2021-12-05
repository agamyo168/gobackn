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

#define ADDRESS "127.0.0.1"
#define PORT 1234
#define MAX_SEQ 4
using namespace std;

// Global Variables
packet networkLayerBuffer[8]={
    {'H','e','l','l','o',' ',' ',' '},
    {'w','o','r','l','d',' ',' ','\n'},
    {'i','t','\'','s','a',' ','m','e'},
    {'m','a','r','i','o','.',' ','\n'},
    {'c','h','r','i','s',' ',' ','\n'},
    {'c','h','e','s','s',' ',' ',' '},
    {'p','r','a','t','t',' ',' ','\n'},
    {'d','i','s','n','e','y','w','o'},
    };
char j = 0; //Network Buffer mod 8
unsigned int server_socket;
unsigned int client_socket;
bool network_layer_enabled = true;
bool time_out_f = false;

// Functions
void from_network_layer(packet *p)
{    
    for (char i = 0; i < 8; i++)
        p->data[i] = networkLayerBuffer[j].data[i];

    j = (j + 1) % 8;
}
static void send_data(seq_nr frame_nr, seq_nr frame_expected, packet buffer[])
{
    /*Construct and send a data frame. */
    frame s;                                            /* scratch variable */
    s.info = buffer[frame_nr];                          /* insert packet into frame */
    s.seq = frame_nr;                                   /* insert sequence number into frame */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1); /* piggyback ack */
    to_physical_layer(&s);                              /* transmit the frame */
    //start_timer(frame_nr);                              /* start the timer running */
}
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
    /*Return true if a <= b < c circularly; false otherwise.*/
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return true;
    else
        return false;
}
void startServer()
{
    cout << "Master started\n";
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    //server_socket = socket(domain,type,protocl)
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS);
    address.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&address, sizeof(address));

    listen(server_socket, 1);

    client_socket = accept(server_socket, NULL, NULL);

    cout << "Connection accepted" << endl;
}
void to_physical_layer(frame *s)
{
    write(client_socket, s, sizeof(frame));
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
void from_physical_layer(frame *s)
{
    read(client_socket, s, sizeof(frame));
    printf("Frame Received\n");
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
void enable_network_layer()
{
    network_layer_enabled = true;
}

void disable_network_layer()
{
    network_layer_enabled = false;
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

int main()
{   startServer();
    seq_nr next_frame_to_send;  /* MAX SEQ > 1; used for outbound stream */
    seq_nr ack_expected;        /* oldest frame as yet unacknowledged */
    seq_nr frame_expected;      /* next frame expected on inbound stream */
    frame r;                    /* scratch variable */
    packet buffer[MAX_SEQ + 1]; /* buffers for the outbound stream */
    seq_nr nbuffered;           /* number of output buffers currently in use */
    seq_nr i;                   /* used to index into the buffer array */
    event_type event;
    enable_network_layer(); /* allow network layer ready events */
    ack_expected = 0;       /* next ack expected inbound */
    next_frame_to_send = 0; /* next frame going out */
    frame_expected = 0;     /* number of frame expected inbound */
    nbuffered = 0; 

    while (true)
    {   wait_for_event(&event);
        switch(event)
        {
        case network_layer_ready: /* the network layer has a packet to send */
        from_network_layer(&buffer[next_frame_to_send]);  
        nbuffered = nbuffered + 1;                             /* expand the sender's window */
        send_data(next_frame_to_send, frame_expected, buffer); /* transmit the frame */
        inc(next_frame_to_send);                               /* advance sender's upper window edge */
        break;
        case frame_arrival:          /* a data or control frame has arrived */
        from_physical_layer(&r); /* get incoming frame from physical layer */
        if (r.seq == frame_expected)
        {
            /*Frames are accepted only in order. */
            //to_network_layer(&r.info); /* pass packet to network layer */
            inc(frame_expected);       /* advance lower edge of receiver's window */

        }
         while (between(ack_expected, r.ack, next_frame_to_send))
            {
                /*Handle piggybacked ack. */
                nbuffered = nbuffered - 1; /* one frame fewer buffered */
                //stop_timer(ack_expected);  /* frame arrived intact; stop timer */
                inc(ack_expected); /* contract sender's window */
            }
            break;
        }
        if (nbuffered < MAX_SEQ)
            enable_network_layer();
        else
            disable_network_layer();
       
    }


    
    
   

    return 0;
}
