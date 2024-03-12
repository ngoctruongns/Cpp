#include <string.h>
#include <stdio.h>

int main()
{
    const char * sample_str = "This is sample string";
    char src[50], dest[50];
    printf("sample string: %s", sample_str);

    // Get length of sample string
    int str_len = strlen(sample_str);
    printf("\nsample string len: %d", str_len);

    // String copy
    strcpy(src,  "This is source");
    strcpy(dest, sample_str);

    printf("\nsrc string: %s", src);
    printf("\ndest string: %s", dest);

    // String cat
    strcat(dest, src);
    printf("\ncat dest string: %s", dest);

    // Revert string
    char revert_str[str_len + 1];
    for (int i = 0; i < str_len; i++)
    {
        revert_str[i]= *(sample_str + str_len - 1 - i);
    }
    
    printf("\nRevert string: %s", revert_str);

    // revert word string
    char * revert_word_str = "";
    char str[80] = "This is www";
    char * token;

    token = strtok(revert_str, " ");
    printf("\nToken: %s", token);

    char revert_str2[str_len + 1];
    int revert_index = 0;
    int index_start = str_len-1;
    int index_stop = 0;
    for (int i = str_len-1; i >= 0 ; i--)
    {
        if (*(sample_str+i)== ' ')
        {
            printf("\nFind space at: %d", i);
            index_stop = i;
            // Copy sub string to revert string
            memcpy(revert_str2 + revert_index, sample_str + i + 1, index_start - index_stop);
            revert_index += (index_start - index_stop);
            index_start = i-1;
            revert_str2[revert_index] = ' ';
            revert_index++;

        }
        // If check to first of string
        if (i==0)
        {
            memcpy(revert_str2 + revert_index, sample_str, index_start + 1);
            printf("Find string done!");
 
        }
    }
    printf("\nRevert2 string: %s", revert_str2);
    
    return 0;
}