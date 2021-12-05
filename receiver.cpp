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
char k = 0;
packet networkLayerBuffer[8];
packet fromNetWorkLayerBuffer[8] ={
    {' ',' ','d','a','t','a',' ','1',},
    {' ',' ','d','a','t','a',' ','2',},
    {' ',' ','d','a','t','a',' ','3',},
    {' ',' ','d','a','t','a',' ','4',},
    {' ',' ','d','a','t','a',' ','5',},
    {' ',' ','d','a','t','a',' ','6',},
    {' ',' ','d','a','t','a',' ','7',},
    {' ',' ','d','a','t','a',' ','8',},

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
      p->data[i] = fromNetWorkLayerBuffer[k].data[i];

    k = (k + 1) % 8;
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
    cout<<"Frame with seq no = "<<r->seq<<" received"<<endl;  
    cout<<"Packet: ";
    for (int i = 0; i < 8; i++)
        cout<< r->info.data[i];
    cout<<endl;
    cout<<"-------------------"<<endl;
}
void to_physical_layer(frame *s)
{
    //Assuming sender frame is just seq_no and packet
    write(server_socket, s, sizeof(frame));

    cout<<"Frame with seq no = "<<s->seq<<" and ack no = "<<s->ack<<" sent"<<endl;  
    cout<<"Packet: ";
    for (int i = 0; i < 8; i++)
        cout<< s->info.data[i];
    cout<<endl;
    cout<<"-------------------"<<endl;
}

void wait_for_event(event_type *event)
{
    if (network_layer_enabled)
    {
        *event = network_layer_ready;
    }
    /*else if (time_out_f)
    {
        *event = timeout;
    }*/
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
    s.ack = (frame_expected + MAX_SEQ ) % (MAX_SEQ + 1);              // 0 1 2 3 0 1 2 3..               /* piggyback ack */
    to_physical_layer(&s);                              /* transmit the frame */
    //start_timer(frame_nr);                              /* start the timer running */
}

int main()
{   /*   initializaitons   */
    startClient();
    event_type event;
    seq_nr nbuffered = 0;           /* number of output buffers currently in use */
    seq_nr frame_expected = 0;      /* next frame expected on inbound stream */
    seq_nr ack_expected = 0;        /* oldest frame as yet unacknowledged */
    seq_nr next_frame_to_send = 0;  /* MAX SEQ > 1; used for outbound stream */
    packet buffer[MAX_SEQ+1]; //Buffer for packets that are sent and can be resent again if timeout (Why SEQ+1?)


    frame r;
    int count = 25;
    
    while(count){
        wait_for_event(&event);
        switch(event)
        {
            case frame_arrival:
            from_physical_layer(&r);
            //idk if this is necessary since I'm not interested in seq no only in ack no
            if(r.seq == frame_expected)
            {
            to_network_layer(&r.info); /* pass packet to network layer */ 
            nbuffered++; //Buffering acks 
            inc(frame_expected);       // /* advance lower edge of receiver's window */ idk what this means
            }

            //we assumed all acks are 100% transmitted
            // while (between(ack_expected, r.ack, next_frame_to_send))
            // {
            //     /*Handle piggybacked ack. */
            //     nbuffered = nbuffered - 1; /* one frame fewer buffered */
            //     //stop_timer(ack_expected);  /* frame arrived intact; stop timer */
            //     inc(ack_expected); /* contract sender's window */
            // }
            break;
            case network_layer_ready:
            from_network_layer(&buffer[next_frame_to_send]);
            nbuffered --;
            send_data(next_frame_to_send,frame_expected,buffer);
            inc(next_frame_to_send);
            break;
        }
         if (nbuffered < MAX_SEQ)
            disable_network_layer();
        else
            enable_network_layer();
    count--;
    }
    return 0;
}