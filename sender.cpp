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
#include <thread>
#include <chrono>
#include <atomic>
#define ADDRESS "127.0.0.1"
#define PORT 1234
#define MAX_SEQ 4
using namespace std;

// Global Variables

packet networkLayerBuffer[8]={
    {'s','e','n','t',' ',' ',' ','1'},
    {'s','e','n','t',' ',' ',' ','2'},
    {'s','e','n','t',' ',' ',' ','3'},
    {'s','e','n','t',' ',' ',' ','4'},
    {'s','e','n','t',' ',' ',' ','5'},
    {'s','e','n','t',' ',' ',' ','6'},
    {'s','e','n','t',' ',' ',' ','7'},
    {'s','e','n','t',' ',' ',' ','8'},
    };
char j = 0; //Network Buffer mod 8
unsigned int server_socket;
unsigned int client_socket;
bool network_layer_enabled = true;
bool time_out_f = false;
thread timerThreads[MAX_SEQ+1];
bool stop_thread[MAX_SEQ+1]={false};
// Functions
void setTimer(bool *time_out,int interval,seq_nr thread_num) {
    if(stop_thread[thread_num]) return;
    this_thread::sleep_for(chrono::milliseconds(interval));
    if(stop_thread[thread_num]) return;
    *time_out = true;
}
void from_network_layer(packet *p)
{    
    for (char i = 0; i < 8; i++)
        p->data[i] = networkLayerBuffer[j].data[i];

    j = (j + 1) % 8;
}
static void send_data(seq_nr frame_nr, packet buffer[])
{
    /*Construct and send a data frame. */
    this_thread::sleep_for(chrono::milliseconds(1000)); //Send one frame per second
    frame s;                                            /* scratch variable */
    s.info = buffer[frame_nr];                          /* insert packet into frame */
    s.seq = frame_nr;                                   /* insert sequence number into frame */
    to_physical_layer(&s);                              /* transmit the frame */
    start_timer(frame_nr);                              /* start the timer running */
    
}
static bool between(seq_nr a, seq_nr b, seq_nr c)
{
    /*Return true if a <= b < c circularly; false otherwise.*/
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return true;
    else
        return false;
}
void start_timer(seq_nr frame_nr)
{   stop_thread[frame_nr] = false;
    timerThreads[frame_nr] = thread(setTimer,&time_out_f,9000,frame_nr);
    timerThreads[frame_nr].detach();
}
void stop_timer(seq_nr ack_expected  )
{
    stop_thread[ack_expected] = true;
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
    //Assuming sender frame is just seq_no and packet
    write(client_socket, s, sizeof(frame));

    cout<<"Frame with seq no = "<<s->seq<<" sent"<<endl;  
    cout<<"Packet: ";
    for (int i = 0; i < 8; i++)
        cout<< s->info.data[i];
    cout<<endl;
    cout<<"-------------------"<<endl;
}
void from_physical_layer(frame *r)
{   
    if(recv(client_socket, r, sizeof(frame),MSG_DONTWAIT) != -1)//Non blocking
    {
    cout<<"Frame with seq no = "<<r->seq<<" and ack no = "<<r->ack<<" received"<<endl;  
    cout<<"Packet: ";
    for (int i = 0; i < 8; i++)
        cout<< r->info.data[i];
    cout<<endl;
    cout<<"-------------------"<<endl;
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
    //Sender has four modes
    //Getting packet from network then send it to receiver
    //receiving acknowledgment from receiver
    //timeout when one ack is missed
    //checksum error
    //first implement the first main two modes
    //network_layer_ready and frame_arrival
    //time out make network layer = false and time out = true then join main thread;
    /*   initializaitons   */ 

    event_type event;
    packet buffer[MAX_SEQ+1]; //Buffer for packets that are sent and can be resent again if timeout (Why SEQ+1?)
    seq_nr next_frame_to_send = 0;  /* MAX SEQ > 1; used for outbound stream */
    seq_nr frame_expected = 0;      /* next frame expected on inbound stream */
    seq_nr nbuffered = 0;           /* number of output buffers currently in use */
    seq_nr ack_expected = 0;        /* oldest frame as yet unacknowledged */
    seq_nr i;                   /* used to index into the buffer array */
    frame r;

    /*   protocol body   */

    while(true)
    {   
        wait_for_event(&event); // returns whether event is sending or receiving acknowledgment
        switch(event)
        {
            //event is: sending frames
            case network_layer_ready:
            from_network_layer(&buffer[next_frame_to_send]);
            nbuffered ++;
            send_data(next_frame_to_send,buffer);
            inc(next_frame_to_send);
            break;
            //event is: receiving acknowledgment
            case frame_arrival:

            from_physical_layer(&r);
            //idk if this is necessary since I'm not interested in seq no only in ack no
            if(r.seq == frame_expected)
            {
            //to_network_layer(&r.info); /* pass packet to network layer */ not really need to
            inc(frame_expected);       // /* advance lower edge of receiver's window */ idk what this means
            }
            while (between(ack_expected, r.ack, next_frame_to_send))
            {
                /*Handle piggybacked ack. */
                nbuffered = nbuffered - 1; /* one frame fewer buffered */
                stop_timer(ack_expected);  /* frame arrived intact; stop timer */
                inc(ack_expected); /* contract sender's window */
            }
            break;
            case cksum_err:
            break;                             /* just ignore bad frames */
            case timeout:                          /* trouble; retransmit all outstanding frames */
            next_frame_to_send = ack_expected; /* start retransmitting here */
            cout<<endl;
            cout<<"--------TIMEOUT---------"<<endl;
            for (i = 1; i <= nbuffered; i++)
            {
                send_data(next_frame_to_send, buffer); /* resend frame */
                inc(next_frame_to_send);                               /* prepare to send the next one */
            }
            time_out_f = false;
            break;

        }
        //This makes sure to stop sending frames when going past window size(MAX_SEQ)
        //Sender becomes listener and waits for acks, if they don't come before time_out the sender resends them
        if(nbuffered < MAX_SEQ)
            enable_network_layer();
        else 
            disable_network_layer();
    }
    

    return 0;
}
