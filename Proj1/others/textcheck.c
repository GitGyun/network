#include <stdio.h>
#include "../functions.h"

int textcheck(const char *filename1, const char *filename2, int len)
{
    FILE *file1, *file2;
    file1 = fopen(filename1, "r");
    file2 = fopen(filename2, "r");
    
    char text1[len], text2[len];
    fread(text1, len, 1, file1);
    fread(text2, len, 1, file2);
    
    int i, check = 1;
    for (i = 0; i < len; i++)
    {
        if (text1[i] != text2[i])
        {
            check = 0;
            break;
        }
    }
    
    fclose(file1);
    fclose(file2);
    
    return check;
}

int main(void)
{
    printf("%x", BUF_SIZE);
        
    return 0;
}
