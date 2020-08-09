#define _GNU_SOURCE_  //使用EPOLLRDHUP的代价
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<assert.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<sys/epoll.h>
#include<sys/types.h>

#define MAXEVENTS 100

//初始化监听sockfd
int InitSock(){

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    if(sockfd == -1)  return -1;

    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(6000);
    ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = bind(sockfd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    if(res == -1)  return -1;

    res = listen(sockfd,5);
    if(res == -1) return -1;

    return sockfd;
}
//获取新的连接，并添加到内核事件表中
void GetClientLink(int epfd,int sockfd){
    struct sockaddr_in cli_addr;
    memset(&cli_addr,0,sizeof(cli_addr));
    socklen_t addrlen = sizeof(cli_addr);

    int c = accept(sockfd,(struct sockaddr*)&cli_addr,&addrlen);
    if(c == -1){
        printf("accept error");
    }
    printf("one link success !!! ");
    
    struct epoll_event events;
    events.events = EPOLLIN | EPOLLRDHUP;
    events.data.fd = c;

    int res = epoll_ctl(epfd,EPOLL_CTL_ADD,c,&events);
    assert(res != -1);

    }
//处理已连接文件描述符的数据
void DealClientData(int fd){
    
}
//处理内核返回的文件描述符
void DealFinishEvents(struct epoll_event *events,int n,int sockfd,int epfd){
    for(int i = 0;i<n;++i){
        if(events[i].data.fd == sockfd){
            GetClientLink(epfd,sockfd);
        }
        else{
            if(events[i].events & EPOLLRDHUP){
                close(events[i].data.fd);
                int res = epoll_ctl(epfd,EPOLL_CTL_DEL,events[i].data.fd,NULL);//删除内核事件表中的这个文件描述符
                if(res  == -1){
                    printf("EPOLL_CTL_DEL  %d  false",events[i].data.fd);
                }
            }
            else{
                DealClientData(events[i].data.fd);
            }
        }
    }
}
int main(){

    int sockfd = InitSock();
    assert(sockfd != -1);

    int epfd = epoll_create(5);
    assert(epfd != -1);

    struct epoll_event events;
    events.events = EPOLLIN;
    events.data.fd = sockfd;

    int res = epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&events);
    assert(res != -1);

    while(1){

        struct epoll_event events[MAXEVENTS];
        int n = epoll_wait(epfd,events,MAXEVENTS,-1); 
        if(n <= 0){
            printf("epoll_wait error\n");
            continue;
        }
        DealFinishEvents(events,n,sockfd,epfd);
    }
}