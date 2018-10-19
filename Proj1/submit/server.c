#include "functions.h"

int encode(char *text, char *code, char *keys)
{
    // Convert keys to integers.
    int i, k[4];
    for (i = 0; i < 4; i++)
        k[i] = tolower((int) keys[i]) - (int) 'a';
        
    i = 0;
    int j = 0;
    int c;
    
    // Encode the text.
    while (text[i] != '\0')
    {
        c = tolower(text[i]);
        
        if (c >= 'a' && c <= 'z')
        {
            code[i] = (c + k[j % 4] - 'a') % 26 + 'a';
            i++;
            j++;
        }
        
        else
        {
            code[i] = text[i];
            i++;
        }
    }
    code[i] = '\0';
    
    return i;
}

int decode(char *code, char *text, char *keys)
{
    // Convert keys to integers.
    int i, k[4];
    for (i = 0; i < 4; i++)
        k[i] = tolower((int) keys[i]) - (int) 'a';
        
    i = 0;
    int j = 0;
    int c;
    
    // Decode the code.
    while (code[i] != '\0')
    {
        c = tolower(code[i]);
        
        if (c >= 'a' && c <= 'z')
        {
            text[i] = (c + (26 - k[j % 4]) - 'a') % 26 + 'a';
            i++;
            j++;
        }
        
        else
        {
            text[i] = code[i];
            i++;
        }
    }
    text[i] = '\0';
    
    return i;
}

int main(int argc, char *argv[])
{
    /* Argument Parsing */
    
    if (argc != 3)
    {
        perror("number of arguments is wrong!");
        exit(0);
    }
    if (strcmp(argv[1], "-p") != 0)
    {
        perror("invalid argument format");
        exit(0);
    }
    
    unsigned short port = (unsigned short)atoi(argv[2]);
    
    
    /* Socket Opening */
    
    int serverfd; // descriptor for listening socket.
    if((serverfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) // Create a socket interface.
    {
        perror("socket creation failed");
        exit(0);
    }
    
    struct sockaddr_in serveraddr, clientaddr; // socket address for server and client.
    serveraddr.sin_family = AF_INET; // using the Internet.
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // allow any IP addresses, network order.
    serveraddr.sin_port = htons(port); // port number, network order.
    
    if (bind(serverfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0) // Bind the socket to the address.
    {
        perror("port binding failed");
        close(serverfd);
        exit(0);
    }
    
    if (listen(serverfd, 1024) < 0) // Set the socket to listen.
    {
        perror("socket listening failed");
        close(serverfd);
        exit(0);
    }
    
    
    /* Service for Clients */
    
    while (1)
    {
        int clientfd; // descriptor for connected socket.
        unsigned clientlen = sizeof(clientaddr); // length of clientaddr structure.
        static char buffer[BUF_SIZE]; // buffer for receiving message.
        static char data[BUF_SIZE-16]; // buffer for internal data processing.
        
        if ((clientfd = accept(serverfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0) // Accept a client and get a connected socket.
        {
            perror("client accept failed");
            exit(0);
        }
        
        if (fork() == 0) // Fork a child.
        {
            close(serverfd); // Child closes its listening socket.
        
            /* Communication and Message Unpacking */
            
            if (read(clientfd, buffer, BUF_SIZE) < 0) // receives a message from client.
            {
                perror("message receive failed");
                goto close;
            }
            
            static struct message rmsg;
            rmsg = *(struct message *)buffer; // received message.
            char keyword[4]; // keyword for the code.
            
            unsigned short op = ntohs(rmsg.op); // operation type, host order.
            strncpy(keyword, rmsg.keyword, 4); // copy the keyword.
            long length = htobe64(rmsg.length); // total length of the message, host order.
            int len_data = (int)length - 16; // length of the data.
            
            
            /* Protocol Specification Checking */
            
            if (length > BUF_SIZE) // Length Check.
            {
                perror("too large message");
                goto close;
            }
            
            if (checksum((char *)&rmsg, (int)length)) // Checksum.
            {
                perror("checksum failed");
                goto close;
            }
            
            
            /* Text Encryption or Decryption */
            int len;
            switch (op)
            {
                // Encryption.
                case 0:
                    len = encode(rmsg.data, data, keyword);
                    break;
            
                // Decryption.
                case 1:
                    len = decode(rmsg.data, data, keyword);
                    break;
                
                // Error.
                default:
                    perror("invalid op format");
                    goto close;
            }
            
            if (len < len_data)
            {
                perror("encryption or decryption failed");
                goto close;
            }
            
            
            /* Message Packing and Communication */
            static struct message smsg;
            
            smsg.op = htons(op); // fill op field, network order.
            smsg.checksum = 0; // initialize checksum field.
            strncpy(smsg.keyword, keyword, 4); // fill keyword field.
            smsg.length = (int64_t)htobe64(length); // fill length field, network order.
            strncpy(smsg.data, data, len_data); // fill data field.
            smsg.checksum = checksum((char *)&smsg, (int)length); // compute the checksum and fill checksum field.
            
            if (write(clientfd, (const void *)&smsg, (int)length) != length) // Send the message.
                perror("message send failed");
            
            
            /* Socket Closing */

            close:
            if (close(clientfd) < 0) // Disconnect the client.
                perror("socket closing failed");
            
            exit(0); // Child exits.
        }
        
        if (close(clientfd) < 0) // Parent closes connected socket.
            perror("socket closing failed");
    }
}
