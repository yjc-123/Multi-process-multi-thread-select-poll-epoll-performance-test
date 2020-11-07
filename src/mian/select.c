#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/select.h>
#include <ctype.h>
#include <libgen.h>


#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))//求数组元素的个数
static inline void msleep(unsigned long ms);

int socket_Server_init(char *listen_ip,int listen_port);
int main (int argc, char **argv){
    int                 listenfd = -1;
    int                 clifd;
    struct sockaddr_in  servaddr;
    struct sockaddr_in  cliaddr;
    socklen_t           len;
    int                serv_port = 8899;
    int                 ch;
    int                 rv ;
    int                 on = 1;
    int                 rdest;
    char                buf[1024];
    int                 i,j;
    int                 found;
    int                 maxfd;
    int                 daemon_run = 0;
    char                *progname = NULL;
    int                 fds_arry[1024];
    fd_set              rdset;
    if( (listenfd = socket_Server_init(NULL, serv_port)) <0)
    {
        printf("ERROR %s server listen on port %d failure\n","59.110.42.24",serv_port);
        return -1;
    }

    printf("%s server start to listen on port %d\n","59.110.42.24",serv_port);

    
    
    for(i=0; i<ARRAY_SIZE(fds_arry); i++)//遍历数组
    {
        fds_arry[i]=-1;//将整个数组初始化为-1，为什么是-1；因为这个数组存放的是文件描述符，系统会生成三个文件描述符0，1，2
    }   
    fds_arry[0] = listenfd;//将第一个数组赋值为listenfd
    //一个tcp的网络链接中包含一个四元组：源ip，目的ip，源端口，目的端口

    for( ; ; )
    {
        FD_ZERO(&rdset);//清空集合
        for(i=0; i<ARRAY_SIZE(fds_arry); i++)
        {
            if( fds_arry[i] < 0)
                continue;
            maxfd = fds_arry[i]>maxfd ? fds_arry[i] : maxfd;
            FD_SET(fds_arry[i], &rdset);//将给定的描述符加入集合
        }

        rv = select(maxfd+1,&rdset,NULL,NULL,NULL);//select函数的返回值是就绪描述符的数目
    if(rv < 0)
    {
        printf("select failure:%s\n",strerror(errno));
        break;
    }
    else if( rv ==0 )
    {
        printf("select get timeout\n");
        continue;
    }
    if( FD_ISSET(listenfd,&rdset) )//判断指定描述符是否在集合中
    {
        if( (clifd=accept(listenfd,(struct sockaddr *)NULL,NULL)) <0)
        {
            printf("accept new client failure:%s\n",strerror(errno));
            continue;
        }

        found = 0;
        for(i=0; i<ARRAY_SIZE(fds_arry);i++)
        {
            if( fds_arry[i]< 0 )
            {
                printf("accept new client [%d] and add it into array\n",clifd);
                fds_arry[i] = clifd;
                found = 1;
                break;
            }
        }
        if(!found)
        {
            printf("accept new client [%d] but full, so refuse it\n",clifd);
            close(clifd);

        }
    }
    else
    {
        for( i=0; i<ARRAY_SIZE(fds_arry);i++)
        {
		//经过了select，rdset已经被将有数据的描述符置1，没有的置0 了。
            if(fds_arry[i]<0 || !FD_ISSET(fds_arry[i],&rdset))
	    {
		    continue;
	    }
	    memset(buf,0 ,sizeof(buf));
            rv = read(fds_arry[i],buf,sizeof(buf));
	    if( rv <0 )
            {
                printf("socket [%d] read failure or get disconnected\n",fds_arry[i]);
                close(fds_arry[i]);
                fds_arry[i]=-1;
		continue;
            }
	    else if(rv ==0){
	    	printf("client connect server is disconnected\n");
		close(fds_arry[i]);
		fds_arry[i] = -1;
		continue;
	    }
            else
            {
                printf("socket [%d] read get %d bytes data:[%s]\n",fds_arry[i],rv,buf);
                for(j=0; j<rv; j++)
		{ 
			buf[j]=toupper(buf[j]);
		}
		if( write(fds_arry[i], buf , rv) <0)
                {
              		printf("socket[%d] write failure:%s\n",fds_arry[i],strerror(errno));
                	close(fds_arry[i]);
                	fds_arry[i] = -1;
                	continue;
		}

            }	
        }
    }
}
cleanup:
    close(listenfd);
    return 0;
}

static inline void msleep(unsigned long ms)
{

    struct timeval tv;
    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
        
    select(0, NULL, NULL, NULL, &tv);
}

int socket_Server_init(char *listen_ip, int listen_port)
{

    struct sockaddr_in  servaddr;
    int     rv = 0;
    int     on = 1;
    int     listenfd;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("use socket()to create a TCP socket failure:%s\n",strerror(errno));
                return -1;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port = htons(listen_port);

    if( !listen_ip )
    {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        if(inet_pton(AF_INET, listen_ip, &servaddr.sin_addr) <=0)
        {
            printf("inet_pton set listen IP address failure\n");
            rv = -2;
            goto cleanup;
        }
    }
    
    if( bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
    {
        printf("socket[%d] bind to port failure:%s\n",listenfd,strerror(errno));
        rv = -3;
        goto cleanup;
    }
      
    if( listen(listenfd,13) < 0)
    {
         printf("use bind to bind tcp socket failure:%s\n",strerror(errno));
         rv = -4;
         goto cleanup;
    }
    
cleanup:
    if(rv < 0)
        close(listenfd);
    else
        rv = listenfd;

    return rv;
}

