/* standard C header */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* system header */
#include <unistd.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


/* protocol header */
#define PROTOCOL_HEAD                   0xffaa

/* operation code */
#define OPT_GET                         0x01
#define OPT_SET                         0x02
#define OPT_HELLO                       0x03

/* property name */
#define PROPERTY_TEMPERATURE_TYPE       0x10
#define PROPERTY_TEMPERATURE_NAME       "temperature"

/* method name */
#define METHOD_HELLO_NAME               "hello"

/* protocol struct */
typedef struct data_protocol
{
    unsigned char head[2];
    unsigned char opt;
    unsigned char type;
    int data;
} data_protocol_t;

/* message and protocol */
#define MAX_MESSAGE                     sizeof(data_protocol_t)

static int connect_device(const char* ip, unsigned short int port)
{
    struct sockaddr_in  device_addr;
    int                 fd  = 0;
    int                 ret = -1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd)
    {
        printf("create socket failed\n");
        goto END;
    }

    (void)memset(&device_addr, 0, sizeof(struct sockaddr_in));
    device_addr.sin_family  = AF_INET;
    device_addr.sin_port    = htons(port);
    (void)inet_pton(AF_INET, ip, &device_addr.sin_addr.s_addr);

    ret = connect(fd, (struct sockaddr*)&device_addr, sizeof(struct sockaddr));
    if (-1 == ret)
    {
        printf("connect socket failed\n");
        goto END;
    }

    return fd;

END:
    if (-1 != fd)
    {
        close(fd);
    }

    return ret;
}

int main(int argc, char* argv[])
{
    if (3 != argc)
    {
        printf("usage: ./client ip port\n");
        return 0;
    }

    int fd = connect_device(argv[1], atoi(argv[2]));
    if (-1 == fd)
    {
        printf("connect device failed\n");
        return 0;
    }

    data_protocol_t packet_message;
    int send_len = 0;
    int recv_msg_len = 0;
    int recv_err_cnt = 0;

    while (1)
    {
        /* send data request */
        (void)memset(&packet_message, 0, sizeof(data_protocol_t));
        packet_message.head[0] = (unsigned char)(PROTOCOL_HEAD >> 8);
        packet_message.head[1] = (unsigned char)(PROTOCOL_HEAD);

        packet_message.type = PROPERTY_TEMPERATURE_TYPE;
        packet_message.opt  = OPT_GET;
        send_len = send(fd, &packet_message, MAX_MESSAGE, 0);
        printf("send packet:    \n \
                    head[0]: %x \n \
                    head[1]: %x \n \
                    type: %x    \n \
                    opt: %x     \n \
                    value: %d   \n", 
                    packet_message.head[0],
                    packet_message.head[1],
                    packet_message.type,
                    packet_message.opt,
                    packet_message.data);

        (void)memset(&packet_message, 0, sizeof(data_protocol_t));
        recv_msg_len = recv(fd, &packet_message, MAX_MESSAGE, 0);
        if (MAX_MESSAGE == recv_msg_len)
        {
            recv_err_cnt = 0;
            if ((packet_message.head[0] != (unsigned char)(PROTOCOL_HEAD >> 8)) 
                || (packet_message.head[1] != (unsigned char)(PROTOCOL_HEAD)))
            {
                printf("recv packet is invalid: \n \
                            head[0]: %x \n \
                            head[1]: %x \n", 
                            packet_message.head[0],
                            packet_message.head[1]);
                continue;
            }

            printf("recv packet:    \n \
                        head[0]: %x \n \
                        head[1]: %x \n \
                        type: %x    \n \
                        opt: %x     \n \
                        value: %d   \n", 
                        packet_message.head[0],
                        packet_message.head[1],
                        packet_message.type,
                        packet_message.opt,
                        packet_message.data);
        }
        else
        {
            if (0 >= recv_msg_len)
            {
                recv_err_cnt++;
                if (recv_err_cnt >= 600)
                {
                    break;
                }
            }
        }

        sleep(5);
    }

    return 0;
}

