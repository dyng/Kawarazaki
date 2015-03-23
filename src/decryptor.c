#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//decrypt the encrypted filename
void namedecrypt(char* name)
{
    int len, key, i;
    len = strlen(name);
    key = len + 1;
    for(i=0;i<len;i++){
        name[i]-=key;
        key--;
    }
}

//size is the source buffer size
//desize is the destination buffer size
//position is the start offset of source in file
//file is the source file
//reference is see the LZSS compression algorithm
unsigned char* decrypt(unsigned int size, unsigned int desize, unsigned int position, FILE* file)
{
    //destination buffer
    unsigned char* desbuffer;
    desbuffer = (char*) malloc(desize*sizeof(char));
    //dictionary buffer
    unsigned char* locbuffer;
    locbuffer = (char*) calloc(0x1000, sizeof(char));

    //seek to the start of source buffer
    fseek(file, position, 0);

    //lrposi is localbuffer read position
    //lwposi is localbuffer write position
    //desposi is destinationbuffer write position
    unsigned int lrposi=0, lwposi=0xFEE, desposi=0, readed=0, nbyte=0;
    unsigned char flag=0;
    unsigned int last;
    unsigned char ch, ch1, ch2;
    unsigned int temp1, temp2;

    while(readed < size){
        //flag is a control flag
        flag = getc(file);
        readed++;

        int i, k, n;
        for(i=0;i<8;i++){
            //get the last bit of flag
            last = flag & 0x1;

            //if last=1, read one more byte
            if(last == 1){
                ch = getc(file);
                readed++;
                //the readed byte is decrypted text
                locbuffer[lwposi]=ch;
                lwposi++;
                lwposi = lwposi & 0xFFF;
                desbuffer[desposi]=ch;
                desposi++;
            //if last=0, read two more bytes
            }else{
                ch1 = getc(file);
                ch2 = getc(file);
                temp1 = (unsigned int) ch1;
                temp2 = (unsigned int) (ch2 & 0xF0)<<4;
                lrposi = temp1 + temp2;
                nbyte = ch2 & 0xF;
                nbyte+=2;
                readed+=2;

                //get decrypted text from dictionary
                for(k=0;k<=nbyte;k++){
                    ch = locbuffer[lrposi];
                    lrposi++;
                    lrposi = lrposi & 0xFFF;
                    locbuffer[lwposi] = ch;
                    lwposi++;
                    lwposi = lwposi & 0xFFF;
                    desbuffer[desposi] = ch;
                    desposi++;
                }
            }
            if(readed >= size)
                break;
            flag = flag>>1;
        }
    }

    free(locbuffer);
    return desbuffer;
}

main(){
    unsigned char* text;
    FILE* output;

    //open the sourcefile "mes.arc"
    FILE* src;
    src = fopen("mes.arc","rb");
    if(src == NULL){
        printf("mes.arc not found!");getchar();
        exit(2);
    }

    //open a newfile to write the order of files compressed in "mes.arc"
    FILE* ord;
    ord = fopen("ord.txt","w+");

    unsigned int num;
    char name[261], path[100];
    unsigned int size, desize;
    long position;

    unsigned char itemp[4];
    unsigned int* ptemp;
    int i;
    long n;
    for(i=0;i<4;i++)
        itemp[i] = fgetc(src);
    ptemp = (unsigned int*) itemp;
    num = *ptemp;

    //create a new directory
    strcpy(path, "text");
    mkdir(path);

    for(n=0;n<num;n++){

        fseek(src, 4+n*0x110, 0);
        fgets(name, 261, src);
        namedecrypt(name);

        fprintf(ord, "%s\n", name);

        //get the source size
        for(i=3;i>=0;i--)
            itemp[i] = fgetc(src);
        ptemp = (unsigned int*) itemp;
        size = *ptemp;

        //get the destination size
        for(i=3;i>=0;i--)
            itemp[i] = fgetc(src);
        ptemp = (unsigned int*) itemp;
        desize = *ptemp;

        //get the start of current buffer
        for(i=3;i>=0;i--)
            itemp[i] = fgetc(src);
        ptemp = (unsigned int*) itemp;
        position = *ptemp;

        //start decryption
        text = decrypt(size, desize, position, src);

        //write decrypted text to file
        strcpy(path, "text\\");
        strcat(path, name);
        output = fopen(path, "wb+");
        fwrite(text, sizeof(char), desize, output);
        fclose(output);
        free(text);

        printf("Extract %s complete.\n", name);
    }

    fclose(ord);
    fclose(src);

    printf("All complete!");

}
