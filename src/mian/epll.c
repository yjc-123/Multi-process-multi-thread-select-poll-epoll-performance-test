#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void handler_events(int epfd,struct epoll_event revs[],int num,int listen_sock)
{
    struct epoll_event ev;
    int i = 0;
    for( ; i < num; i++ )
    {
    int fd = revs[i].data.fd;

    // 如果是监听文件描述符，则调用accept接受新连接

    if( fd == listen_sock && (revs[i].events & EPOLLIN) )
    {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int new_sock = accept(fd,(struct sockaddr *)&client,&len);

        if( new_sock < 0 )
        {
        perror("accept fail ... \n");
        continue;
        }

       printf("get a new link![%s:%d]\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));

       //因为只是一个http协议：连接成功后，下面就是要 请求和响应 
       // 而服务器端响应之前：要先去读客户端要请求的内容

       ev.events = EPOLLIN;
       ev.data.fd = new_sock;
       epoll_ctl(epfd,EPOLL_CTL_ADD,new_sock,&ev);

       continue;
    }

    // 如果是普通文件描述符，则调用read提供读取数据的服务

    if(revs[i].events & EPOLLIN)
    {
        char buf[10240];
        ssize_t s = read(fd,buf,sizeof(buf)-1);
        if( s > 0 )// 读成功了
        {
        buf[s] = 0;
        printf(" %s ",buf);

        // 读成功后，就是要给服务端响应了
        // 而这里的事件是只读事件，所以要进行修改

        ev.events = EPOLLOUT;// 只写事件
        ev.data.fd = fd;
        epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);// 其中EPOLL_CTL_MOD 表示修改

        }

        else if( s == 0 )
        {
        printf(" client quit...\n ");
        close(fd);// 这里的fd 就是 revs[i].fd
        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);// 连接关闭，那么就要把描述该连接的描述符关闭
        }
        else// s = -1 失败了
        {   
        printf("read fai ...\n");
        close(fd);// 这里的fd 就是 revs[i].fd
        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);// 连接关闭，那么就要把描述该连接的描述符关闭
        }
        continue;
    }

    // 服务器端给客户端响应: 写

    if( revs[i].events & EPOLLOUT )
    {
        const char* echo = "HTTP/1.1 200 ok \r\n\r\n<html>hello epoll server!!!</html>\r\n";
        write(fd,echo,strlen(echo));
        close(fd);
        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
    }
    }
}

int startup( int port )
{
    // 1. 创建套接字
    int sock = socket(AF_INET,SOCK_STREAM,0);//这里第二个参数表示TCP
    if( sock < 0 )
    {
    perror("socket fail...\n");
    exit(2);
    }

    // 2. 解决TIME_WAIT时，服务器不能重启问题；使服务器可以立即重启
    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);// 地址为任意类型
    local.sin_port = htons(port);// 这里的端口号也可以直接指定8080
    // 3. 绑定端口号

    if( bind(sock,(struct sockaddr *)&local,sizeof(local)) < 0 )
    {
    perror("bind fail...\n");
    exit(3);
    }

    // 4. 获得监听套接字
    if( listen(sock,5) < 0 )
    {
    perror("listen fail...\n");
    exit(4);
    }
    return sock;
}

int main(int argc,char* argv[] )
{

    // 1. 创建一个epoll模型: 返回值一个文件描述符

    int epfd = epoll_create(256);
    if( epfd < 0 )
    {
    perror("epoll_create fail...\n");
    return 2;
    }

    // 2. 获得监听套接字

   int listen_sock = startup(atoi("8899"));//端口号传入的时候是以字符串的形式传入的,需要将其转为整型


    // 3. 初始化结构体----监听的结构列表

    struct epoll_event  ev;
    ev.events = EPOLLIN;//关心读事件
    ev.data.fd = listen_sock;// 关心的描述文件描述符

    // 4. epoll的事件注册函数---添加要关心的文件描述符的只读事件

    epoll_ctl(epfd,EPOLL_CTL_ADD,listen_sock,&ev);

    struct epoll_event revs[128];
    int n = sizeof(revs)/sizeof(revs[0]);

    int timeout = 3000;
    int num = 0;


    while(1)
    {

       // 5 . 开始调用epoll等待所关心的文件描述符集就绪

       switch( num = epoll_wait(epfd,revs,n,timeout) )
       {
       case 0:// 表示词状态改变前已经超过了timeout的时间
           printf("timeout...\n");
           continue;
       case -1:// 失败了
           printf("epoll_wait fail...\n");
           continue;
       default: // 成功了

           handler_events(epfd,revs,num,listen_sock);
           break;
       }
    }
    close(epfd);
    close(listen_sock);
    return 0;
}

