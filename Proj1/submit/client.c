#include "functions.h"

void parse(int argc, char *argv[], char *args[])
{
    char *list[4][2];
    int i, j;
    for (i = 0; i < 4; i++)
    {
        args[i] = "";
        
        for (j = 0; j < 2; j++)
            list[i][j] = argv[2*i + j];
    }
    
    
    for (i = 0; i < 4; i++)
    {
        char *argtype = list[i][0];
        char *arg = list[i][1];
        
        if (strcmp(argtype, "-h") == 0)
        {
            if (strcmp(args[0], "") != 0)
                goto ERROR;
                
            args[0] = arg;
        }
        
        else if (strcmp(argtype, "-p") == 0)
        {
            if (strcmp(args[1], "") != 0)
                goto ERROR;
                
            args[1] = arg;
        }
        
        else if (strcmp(argtype, "-o") == 0)
        {
            if (strcmp(args[2], "") != 0)
                goto ERROR;
                
            args[2] = arg;
        }
        
        else if (strcmp(argtype, "-k") == 0)
        {
            if (strcmp(args[3], "") != 0)
                goto ERROR;
                
            args[3] = arg;
        }
        
        else
            goto ERROR;
    }
    
    return;

ERROR: 
    perror("argument format is invalid!\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    /* Argument Parsing */
    
    if (argc != 9)
    {
        perror("number of arguments is wrong!\n");
        exit(0);
    }
    
    char **argv_core = argv + 1;
    char *args[8];
    
    parse(argc, argv_core, args); // parses arguments into args.
    
    unsigned short op = (unsigned short)atoi(args[2]); // operation type.
    char *keyword = args[3]; // keyword for the code.
    
    
    /* Message Packing */
    
    static char data[BUF_SIZE-16]; // buffer for sending data.
    int len_data = read(0, data, BUF_SIZE - 16); // Get a string from the stdin.
    long length = (long)len_data + 16; // total length of the message.
   
    static struct message smsg; // message for send.
    
    smsg.op = htons(op); // fill op field in network order.
    smsg.checksum = 0; // initialize checksum field.
    strncpy(smsg.keyword, keyword, 4); // fill keyword field.
    smsg.length = (int64_t)htobe64(length); // fill length field in network order.
    strncpy(smsg.data, data, len_data); // fill data field.
    smsg.checksum = checksum((char *)&smsg, (int)length); // compute checksum and fill checksum field.
    
    
    /* Socket Opening and Conncetion Establishment */
    
    int clientfd; // socket descriptor for communication.
    if((clientfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) // Get a socket descriptor.
    {
        perror("socket creation failed\n");
        exit(0);
    }
    
    unsigned short port = htons((unsigned short)atoi(args[1])); // port number of the server, network order.
    long host = inet_addr(args[0]); // IP address of the server, network order.

    struct sockaddr_in clientaddr;
    clientaddr.sin_family = AF_INET; // using the Internet.
    clientaddr.sin_addr.s_addr = host; // server address.
    clientaddr.sin_port = port; // server port.
    
    if (connect(clientfd, (struct sockaddr*) &clientaddr, sizeof(clientaddr)) < 0) // Make a connection to the server.
    {
        perror("server connection failed");
        exit(0);
    }
    
    
    /* Communication */
    
    int len_sent; // total length of sent message.
    if ((len_sent = write(clientfd, (const void *)&smsg, (int)length)) != length) // Send the message to the server.
    {
        fprintf(stderr, "%d %ld\n", len_sent, length);
        perror("message send failed");
        exit(0);
    }
    
    int len_recv; // total length of recieved message.
    static char buffer[BUF_SIZE]; // buffer for receiving message.
    if ((len_recv = read(clientfd, buffer, (int)length)) < 0) // Recieve a message from the server.
    {
        perror("data recieve failed");
        exit(0);
    }
    
    
    /* Message Unpacking and Socket Closing */
    
    static struct message rmsg;
    rmsg = *(struct message *)buffer; // message recieved.
    
    if (checksum((char *)&rmsg, len_recv) == 0) // checksum
        write(1, rmsg.data, len_recv - 16); // write data to the stdout.
    
    else
        perror("checksum failed");
    
    if (close(clientfd) < 0) // Close the socket interface.
    {
        perror("socket closing failed");
        exit(0);
    }
    
    return 0;
}
