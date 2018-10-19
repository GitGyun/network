long parse_addr(char *host)
{
    int i = 0;
    int pos = 0;
    int j;
    
    char buff[3];
    int count = 0;
    long addr[4];
    
    while (1)
    {
        for (j = 0; j < 2; j++)
            buff[j] = '\0';
        
        while (host[i] != '.' && host[i] != '\0')
        {
            assert(i - pos <= 2 && i <= 15);
            buff[i - pos] = host[i];
            i++;
        }
        addr[count++] = (long) atoi(buff);
        
        if (host[i] == '\0')
            break;
        
        i++;
        pos = i;
    }
    
    if (count != 4)
    {
        printf("address format is invalid!\n");
        exit(0);
    }
    
    return addr[0] * (2 << 23) + addr[1] * (2 << 15) + addr[2] * (2 << 7) + addr[3];
}
