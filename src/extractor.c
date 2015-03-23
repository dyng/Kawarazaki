#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//extract Shift-JIS characters from sourcefile
void extract(FILE* src, FILE* des){
    unsigned char byte;
    char str[2000];
    unsigned int* strucnum;

    int taken, i;

    int ftmp;
    unsigned char numtmp[4];

    //analyse the file header
    strucnum = (unsigned int*) numtmp;
    ftmp = fread(numtmp, sizeof(char), 4, src);
    if(ftmp != 4){
        return;
    }
    ftmp = fseek(src, (*strucnum)*4+4, SEEK_SET);
    if(ftmp != 0){
        return;
    }

    //arrive at the start of textblock
    while((taken=fgetc(src)) != EOF){
        byte = (unsigned char) taken;

        //if byte is a Shift-JIS character
        if(((0x81<=byte)&&(byte<=0x9f))||((0xe0<=byte)&&(byte<=0xef))){
            i = 0;
            str[i++] = byte;
            while((taken=fgetc(src))!=0x0){
                byte = (unsigned char) taken;
                str[i++] = byte;
            }
            str[i] = '\0';

            fprintf(des, "%s\n\n", str);
        //else just ignore it
        }else{
            switch(byte){
                case 0x16:
                case 0x19:
                case 0x15:
                case 0x1a:
                case 0x14:
                case 0x32:
                    fseek(src, 4, SEEK_CUR);
                    break;
                case 0x33:
                    while((taken=fgetc(src))!=0x0);
                    break;
            }
        }
    }
}

main(){
    FILE *ord, *src, *des;
    char srcname[50];
    char desname[50];
    char path[50];

    int filenamelen;

    ord = fopen("ord.txt", "r");
    if(ord == NULL){
        printf("ord.txt not found!");getchar();
        exit(2);
    }

    while(!feof(ord)){
        //set the destinationfile's name
        fscanf(ord, "%s\n", srcname);
        filenamelen = strlen(srcname)-4;

        strncpy(desname, srcname,filenamelen);
        desname[filenamelen] = '\0';
        strcat(desname, ".txt");
        strcpy(path, "rawtext\\");
        mkdir(path);
        strcat(path, desname);

        //open the sourcefile
        src = fopen(srcname, "rb");
        if(src == NULL){
            printf("%s not found!", srcname);getchar();
            exit(2);
        }
        //create the destinationfile
        des = fopen(path, "w+");

        //extract the text from sourcefile and write to destinationfile
        extract(src, des);

        fclose(src);
        fclose(des);

        printf("Extract %s complete.\n", desname);
    }

    fclose(ord);
    printf("All complete!");
}
