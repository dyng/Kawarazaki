#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct{
    //address in buffer
    int address;

    //address jumpto in file
    int jumpaddress;

} Branch;

void stackpop(unsigned char* buffer, int offset, Branch* pstack){
    int newjumpaddress, bufferaddress, i;
    unsigned char itmp[4];
    unsigned char* ptmp;

    ptmp = (unsigned char*) &newjumpaddress;
    newjumpaddress = pstack->jumpaddress + offset;
    for(i=0;i<4;i++){
        itmp[i] = ptmp[3-i];
    }

    bufferaddress = pstack->address + 1;
    memcpy(buffer+bufferaddress, itmp, 4);
}

int integrate(FILE* src, FILE* text, FILE* des){
    int filesize, scount, count, bindex, baseaddress;
    int offset, orilen, tranlen;
    unsigned char* buffer;
    unsigned char word[2000];
    unsigned char translation[2000];

    Branch logicstack_14[500];
    Branch* pstack_14 = logicstack_14;
    Branch logicstack_15[100];
    Branch* pstack_15 = logicstack_15;
    Branch logicstack_16[100];
    Branch* pstack_16 = logicstack_16;
    Branch logicstack_1a[100];
    Branch* pstack_1a = logicstack_1a;

    unsigned int* pint;
    unsigned char numtmp[4];
    unsigned char* pchar;
    int curposi, jaddr, i, k, n;

    //get filesize
    fseek(src, 0, SEEK_END);
    filesize = ftell(src);
    fseek(src, 0, SEEK_SET);
    if(filesize <= 4)
        return 0;

    //initialization
    buffer = malloc(sizeof(char)*(filesize+4000));
    pint = (unsigned int*) numtmp;
    bindex = 0;
    offset = 0;
    scount = 0;

    //if translation doesn't exist
    if(text == NULL){
        fread(buffer, sizeof(char), filesize, src);
        fwrite(buffer, sizeof(char), filesize, des);
        return 2;
    }

    //get block numbers
    fread(numtmp, sizeof(int), 1, src);
    memcpy(buffer+bindex, pint, 4);
    bindex += 4;

    //copy the header block
    fread(buffer+bindex, sizeof(int), (*pint), src);
    bindex += (*pint)*4;
    baseaddress = bindex;

    while((count=readword(word, src))){
        switch(word[0]){
            case 0x19:
                //modify jump address
                jaddr = ftell(src)-5+offset-baseaddress;
                memcpy(buffer+4*scount+4, &jaddr, 4);
                scount++;

                //write to buffer
                memcpy(buffer+bindex, word, count);
                bindex+=count;
                break;

            case 0x16:
                for(i=0;i<4;i++){
                    numtmp[i] = word[4-i];
                }
                //push a new branch
                pstack_16++;
                pstack_16->address = bindex;
                pstack_16->jumpaddress = (*pint);

                //write to buffer
                memcpy(buffer+bindex, word, count);
                bindex+=count;
                break;

            case 0x15:
                for(i=0;i<4;i++){
                    numtmp[i] = word[4-i];
                }
                //push a new branch
                pstack_15++;
                pstack_15->address = bindex;
                pstack_15->jumpaddress = (*pint);

                //write to buffer
                memcpy(buffer+bindex, word, count);
                bindex+=count;
                break;

            case 0x1a:
                for(i=0;i<4;i++){
                    numtmp[i] = word[4-i];
                }
                //push a new branch
                pstack_1a++;
                pstack_1a->address = bindex;
                pstack_1a->jumpaddress = (*pint);

                //write to buffer
                memcpy(buffer+bindex, word, count);
                bindex+=count;
                break;

            case 0x14:
                for(i=0;i<4;i++){
                    numtmp[i] = word[4-i];
                }
                //push a new branch
                pstack_14++;
                pstack_14->address = bindex;
                pstack_14->jumpaddress = (*pint);

                //write to buffer
                memcpy(buffer+bindex, word, count);
                bindex+=count;
                break;

            default:
                if(((0x81<=word[0])&&(word[0]<=0x9f))||((0xe0<=word[0])&&(word[0]<=0xef))){
                    //read translation file
                    fscanf(text, "%s\n\n", translation);

                    //set offset
                    orilen = count;
                    tranlen = strlen(translation)+1;
                    offset += tranlen - orilen;

                    //write to buffer
                    memcpy(buffer+bindex, translation, tranlen);
                    bindex+=tranlen;

                }else{
                    //write to buffer
                    memcpy(buffer+bindex, word, count);
                    bindex+=count;
                }
        }

        //pop possiblely elements
        curposi = ftell(src)-baseaddress;
        while((pstack_14!=logicstack_14)&&(pstack_14->jumpaddress<=curposi)){
            stackpop(buffer, offset, pstack_14);
            pstack_14--;
        }
        while((pstack_15!=logicstack_15)&&(pstack_15->jumpaddress<=curposi)){
            stackpop(buffer, offset, pstack_15);
            pstack_15--;
        }
        while((pstack_16!=logicstack_16)&&(pstack_16->jumpaddress<=curposi)){
            stackpop(buffer, offset, pstack_16);
            pstack_16--;
        }
        while((pstack_1a!=logicstack_1a)&&(pstack_1a->jumpaddress<=curposi)){
            stackpop(buffer, offset, pstack_1a);
            pstack_1a--;
        }
    }

    //write buffer to destination file
    fwrite(buffer, sizeof(char), bindex, des);
    return 1;
}

//read a word, return readed bytes number in return value and word in buffer
int readword(unsigned char* buffer, FILE* src){
    unsigned char byte;
    int index;

    if(feof(src))
        return 0;

    byte = fgetc(src);

    if(((0x81<=byte)&&(byte<=0x9f))||((0xe0<=byte)&&(byte<=0xef))){
        index = 0;
        buffer[index++] = byte;
        while((byte=fgetc(src))!=0x0){
            buffer[index++] = byte;
        }
        buffer[index++] = '\0';
        return index;

    }else{
        switch(byte){
            case 0x16:
            case 0x19:
            case 0x15:
            case 0x1a:
            case 0x14:
            case 0x32:
                buffer[0] = byte;
                for(index=1;index<5;index++){
                    buffer[index] = fgetc(src);
                }
                return index;

            case 0x33:
                index = 0;
                buffer[index++] = byte;
                while((byte=fgetc(src))!=0x0){
                    buffer[index++] = byte;
                }
                buffer[index++] = '\0';
                return index;

            default:
                buffer[0] = byte;
                return 1;
        }
    }
}

main(){
    FILE *ord, *src, *text, *des;
    char srcfilename[50];
    char texfiletname[50];
    char path[100];
    int namelen;

    ord = fopen("ord.txt", "r");
    if(ord == NULL){
        printf("ord.txt not found!");getchar();
        exit(2);
    }

    strcpy(path, "integratedmes\\");
    mkdir(path);

    while(!feof(ord)){

        //get filename
        fscanf(ord, "%s\n", srcfilename);
        strcpy(path, "integratedmes\\");
        strcat(path, srcfilename);

        //set textfilename
        namelen = strlen(srcfilename)-4;
        strncpy(texfiletname, srcfilename, namelen);
        texfiletname[namelen] = '\0';
        strcat(texfiletname, ".txt");

        //original mes file
        src = fopen(srcfilename, "rb");
        if(src == NULL){
            printf("%s not found!", srcfilename);getchar();
            exit(2);
        }

        //translation file
        text = fopen(texfiletname, "r");

        //destination file
        des = fopen(path, "wb+");

        integrate(src, text, des);

        //close all files
        fclose(src);
        if(text != NULL)
            fclose(text);
        fclose(des);

        printf("Integrate %s complete.\n", srcfilename);
    }

    fclose(ord);
    printf("All complete!");
}
