#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include "rssi.h"
#include "packet.h"
#include "time_util.h"
 

// TODO fix indentation, test, relocate functions, 

bool shutdown_flag = false;

void kill_handler(int signo) {
    (void)signo;
    shutdown_flag = true;
}

typedef struct _queue_elem{
    void* val;
    struct _queue_elem* next;
} queue_elem;

typedef struct _queue{
    queue_elem* first;
    queue_elem* last;
} queue;

queue* create_queue(){
    return calloc(1,sizeof(queue));
}

void enqueue(queue* q, queue_elem* new_elem){
    if(!q || !new_elem){
        return;
    }

    if(!q->first){
        q->first = new_elem;
    } else {
        q->last->next = new_elem;
    }
    q->last = new_elem;
    new_elem->next = NULL;
}

queue_elem* dequeue(queue* q){
    if(!q || !q->first)
        return NULL;

    if(q->first == q->last){
        q->last = NULL;
    }
    queue_elem* elem = q->first;
    q->first = q->first->next;
    return elem;
}

queue_elem* peek_queue(queue* q){
    return q->first;
}

typedef struct _msg{
    size_t len;
    uint8_t data[255];
} msg;

typedef struct _backoff_struct{
    uint8_t attempts;
    uint8_t backoff_ms;
    struct timespec start_backoff;
} backoff_struct;

typedef struct _node_msg_struct{
    msg message;
    backoff_struct backoff;
} node_msg_struct;

node_msg_struct* create_msg(size_t len, char* data){
    node_msg_struct* new_msg_struct = malloc(sizeof(node_msg_struct));
    new_msg_struct->message.len = len;
    strncpy((char*)new_msg_struct->message.data, data, 255);
    new_msg_struct->backoff.attempts = 0;
    new_msg_struct->backoff.backoff_ms = 0;
    struct timespec time = {0, 0};
    new_msg_struct->backoff.start_backoff = time;
    
    return new_msg_struct;
}

queue* msg_queue;
void* collect_user_input(){
    char msg_data[256];
    msg_data[255] = '\n';
    size_t len = 0;

    while(1){
       scanf("%255s%n", msg_data, (uint32_t*)&len); 
       node_msg_struct* new_msg_struct = create_msg(len, msg_data);
       queue_elem* elem = malloc(sizeof(queue_elem));
       elem->val = (void*)new_msg_struct;
       enqueue(msg_queue, elem);
    }
}

size_t random_number_between(size_t min, size_t max) {
    assert(min <= max);
    srand(time(NULL));
    return min + (rand() % (int)(max - min + 1));
}

void set_new_backoff(backoff_struct* backoff){
    backoff->attempts++;
    backoff->backoff_ms = random_number_between(0, (1 << backoff->attempts)-1);
    clock_gettime(CLOCK_MONOTONIC_RAW, &backoff->start_backoff);
}

bool check_backoff_timeout(backoff_struct* backoff){
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC_RAW, &current);
    
    size_t delta_ms = (current.tv_sec - backoff->start_backoff.tv_sec) * 1000 +
        (current.tv_nsec - backoff->start_backoff.tv_nsec) / 1000000;

    return delta_ms >= backoff->backoff_ms;
}

bool send_ready() {
    node_msg_struct* msg = peek_queue(msg_queue)->val;

    if(!msg || !check_backoff_timeout(&msg->backoff)){
        return false;
    }

    if(detect_RSSI()){
        goto backoff;
    }

    send_packet(&msg->message);
        
    if(detect_RSSI()){
        goto backoff;
    }

    dequeue(msg_queue);
    return true;

backoff:
    // TODO try to receive?
    set_new_backoff(&msg->backoff);
    return false;
}
            

void listen(){
    uint8_t packet[255] = {0};
    if(detect_RSSI()){

      enable_preamble_detection();

      uint8_t recv_bytes;
      if((recv_bytes = receive_packet(packet)) == 0) {
          cc1200_cmd(SIDLE);
          cc1200_cmd(SFRX);
      }

      disable_preamble_detection();
    }

}

int main() {
    msg_queue = create_queue();
    pthread_t user_input_thread;
    pthread_create(&user_input_thread, NULL, collect_user_input, NULL);
    //pthread_create(&listen_thread, NULL, collect_user_input, NULL);
    while(!shutdown_flag) {
        struct timespec start_time;
        clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
        while(!hit_timeout(10, &start_time)){
            listen();
        }
        send_ready();
    }
}

