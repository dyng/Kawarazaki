#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXMATCH 18

typedef struct {
    //file name
    char name[32];

    //encrypted text size
    unsigned int size;

    //original text size
    unsigned int desize;

    //position in mes.arc
    unsigned int position;

    //buffer for encrypted text
    unsigned char* buffer;
} Node;

//return encrypted filename
void encryptfn(char* filename){
    int i, n, key;
    n = strlen(filename);
    key = n+1;
    for(i=0;i<n;i++){
        filename[i]+=key;
        key--;
    }
}

//return match position an length
void matcher(int front, int rear, unsigned char initchar, unsigned char* locbuffer, FILE* file, int* bundle){
    int mlen, mposi;
    int try;
    unsigned char mstring[20];

    //temp int variable for fgetc()
    int taken;

    mstring[0] = initchar;
    mlen = 0;
    mposi = 0;
    try = front;

    while(try>=rear){
        if(mstring[0]==locbuffer[try]){
            if(!memcmp(mstring, locbuffer+try, mlen+1)){

                //update matchinfo
                locbuffer[front+1] = mstring[mlen];
                front++;
                mlen++;
                mposi = try;
                
                //prepare for the next matching
                taken = fgetc(file);
                mstring[mlen] = (unsigned char) taken;

                //if reached the maximum match length
                if(mlen>=MAXMATCH)
                    break;

                //if reached the EOF
                if(taken == EOF) //because unsigned char can't distinguish EOF from 0xFF
                    break;
            }else{
                try--;
            }
        }else{
            try--;
        }
    }

    //if no match totally, updata locbuffer by the initchar
    if(mlen == 0)
        locbuffer[front+1] = initchar;

    bundle[0] = mposi;
    bundle[1] = mlen;
}

//return encrypted text in buffer, and text size in bindex
unsigned int encrypttext(FILE* file, int desize, unsigned char* buffer){
    unsigned char flag;
    unsigned char block[20];
    unsigned int bindex = 0;
    unsigned int blen;

    int mlen, mposi;
    unsigned char initchar;

    int front, rear;
    unsigned char* locbuffer;

    //temp variables
    int i, k;
    unsigned int btemp1, btemp2;
    unsigned char byte1, byte2;
    int bundle[2];

    //initialization
    locbuffer = malloc(sizeof(char)*(desize+10));
    if(locbuffer==NULL){
        printf("Memory Insuffient\nExit...");getchar();
        exit(2);
    }
    locbuffer[0] = 0x0;

    front = rear = 0;

    while(front < desize){
        flag = 0x0;
        blen = 1;
        for(i=0;i<8;i++){
            //initchar will not encouter EOF
            initchar = fgetc(file);

            matcher(front, rear, initchar, locbuffer, file, bundle);
            //match position
            mposi = bundle[0];
            //match length
            mlen = bundle[1];

            //set the [i]th bit of flag to 0
            if(mlen >= 3){
                btemp1 = (mposi+0xFED) & 0xFFF;
                btemp2 = (btemp1 & 0xF00)>>4;
                byte1 = (char) (btemp1 & 0xFF);
                byte2 = (char) btemp2 + mlen - 3;

                block[blen++] = byte1;
                block[blen++] = byte2;

                for(k=0;k<mlen;k++){
                    front++;
                    if((front-rear) > 0xFFF)
                        rear++;
                }

                //seek back for the one byte not matched
                fseek(file, -1L, SEEK_CUR);

            //set the [i]th bit of flag to 1
            }else{
                block[blen++] = initchar;
                flag = flag|(0x1<<i);

                front++;
                if((front-rear) > 0xFFF)
                    rear++;

                //seek back for mlen bytes, because when a match hits, fgetc a new byte and mlen++
                fseek(file, -mlen, SEEK_CUR);
            }

            //EOF is a key problem of this program
            if(front >= desize)
                break;
        }
        block[0] = flag;

        memcpy(buffer+bindex, block, blen);
        bindex += blen;
    }

    free(locbuffer);
    return bindex;
}

//write the header with a block of 0x110 bytes containing the structure informations
void wnheader(Node* node, FILE* file){
    int i, n;
    unsigned char* byte;
    char enfname[260] = {0};

    //encrypted filename
    strcpy(enfname, node->name);
    encryptfn(enfname);
    fwrite(enfname, sizeof(char), 260, file);

    //encrypted data size
    byte = (unsigned char*) &(node->size);
    for(n=3;n>=0;n--){
        fputc((int)byte[n], file);
    }

    //decrypted data size
    byte = (unsigned char*) &(node->desize);
    for(n=3;n>=0;n--){
        fputc((int)byte[n], file);
    }

    //data position in mes.arc
    byte = (unsigned char*) &(node->position);
    for(n=3;n>=0;n--){
        fputc((int)byte[n], file);
    }
}

//write the encrypted text
void wndata(Node* node, FILE* file){
    fwrite(node->buffer, sizeof(char), node->size, file);
}

main(){
    FILE* ford = fopen("ord.txt", "r");
    FILE* fscr;
    FILE* fdes = fopen("mes.arc", "wb+");

    if(ford == NULL){
        printf("ord.txt not found!");getchar();
        exit(2);
    }

    char fname[32];
    int fnum=0;

    //temp variable
    int n, i;
    unsigned long curposition;

    //get filename for encryption and structure number in fnum
    while(fgets(fname, 32, ford)){
        fnum++;
    }
    rewind(ford);

    curposition = 0x110*fnum+4 ;
    //allocate memory for nodelist
    Node* lnode = malloc(fnum*sizeof(Node));

    for(n=0;n<fnum;n++){
        fscanf(ford, "%s\n", fname);
        fscr = fopen(fname, "rb");

        //set node.name
        strcpy(lnode[n].name, fname);

        //get source text size
        fseek(fscr, 0L, SEEK_END);
        lnode[n].desize = ftell(fscr);
        fseek(fscr, 0L, SEEK_SET);

        lnode[n].buffer = malloc(lnode[n].desize * sizeof(char));
        if(lnode[n].buffer==NULL){
            printf("Memory Insuffient\nExit...");getchar();
            exit(2);
        }
        
        //set node.size
        lnode[n].size = encrypttext(fscr, lnode[n].desize, lnode[n].buffer);

        //set node.position
        lnode[n].position = curposition;
        curposition += lnode[n].size;

        fclose(fscr);
        printf("Pack %s complete.\n", lnode[n].name);
    }

    //the first 4bytes int indicates the number of structures
    fwrite(&fnum, sizeof(char), 4, fdes);
    //write structure header
    for(i=0;i<fnum;i++){
        wnheader(lnode+i, fdes);
    }
    //write data block
    for(i=0;i<fnum;i++){
        wndata(lnode+i, fdes);
    }

    fclose(fdes);
    free(lnode);
    fclose(ford);

    printf("All complete!");
}
