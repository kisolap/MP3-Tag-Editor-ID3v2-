#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Main Header - 10 bytes (optional - extended header)
struct mainHeader{
    char mainID[3];
    char version[2];
    char flags[1];
    unsigned char size[4];
};

// Frame Header - 10 bytes (without frame directly)
struct frameHeader{
    char frameID[4];
    unsigned char size[4];
    char flags[2];
};

int getSizeTag(const unsigned char *sizeBytes){
    int size = 0;
    for (int i = 0; i < 4; i++){
        size += sizeBytes[i] * pow(2, 7 * (3 - i)); // ID3v2 size (4 * %0xxxxxxx)
        // most Significant it's 0
    }
    return size;
}

int getSizeFrame(const unsigned char *sizeBytes){
    int size = 0;
    for (int i = 0; i < 4; i++){
        size += sizeBytes[i] * pow(2, 7 * (3 - i)); // ID3v2 size (4 * %0xxxxxxx)
        // most Significant it's 0
    }
    return size;
}

void setSizeFrame(unsigned char *sizeBytes, long long fullSize){
    for (int i = 0; i < 4; i++){
        sizeBytes[i] = fullSize / pow(2, 7 * (3 - i));
        fullSize -= sizeBytes[i] / pow (2, 7 * (3 - i));
    }
}

void printFrames(FILE* track);
void printFrame(FILE *track, char *frame);
void setFrame(FILE *track, FILE *output, char *frameName, char *frameValue);

int main(int argc, char *argv[]) {
    char commands[3][15] = {'\0'};
    char arguments[3][50] = {'\0'};

    // app.exe 1 --filepath=Song.mp3 --set=COMM --value=Test

    int k;
    int m = 0;

    for (int i = 1; i < argc; i++) {
        for (int j = 0; j < strlen(argv[i]); j++) {
            k = 0;
            while (argv[i][j] != '=') {
                commands[i - 1][k] = argv[i][j];
                k++;
                j++;
            }
            commands[i - 1][k] = '\0';
            k = 0;
            j += 1;
            while (argv[i][j] != '\0') {
                arguments[i - 1][k] = argv[i][j];
                k++;
                j++;
            }
            arguments[i - 1][k] = '\0';
        }
        m += 1;
    }

    FILE *track = fopen(arguments[0], "r+b");
    FILE *outputTrack = fopen("Track.mp3", "w+b");

    for (int i = 1; i < 3; i++) {
        if (strcmp(commands[i], "--show") == 0) {
            printFrames(track);
        } else if (strcmp(commands[i], "--get") == 0) {
            printf("%s %s %s \n", commands[0], commands[1], commands[2]);
            printf("%s %s %s \n", arguments[0], arguments[1], arguments[2]);
            printFrame(track, arguments[1]);
        } else if (strcmp(commands[i], "--set") == 0){
            printf("%s %s %s \n", commands[0], commands[1], commands[2]);
            printf("%s %s %s \n", arguments[0], arguments[1], arguments[2]);
            setFrame(track, outputTrack, arguments[1], arguments[2]);
        }
    }
}

void setFrame(FILE *track, FILE *output, char *frameName, char *frameValue) {
    fseek(track, 0, SEEK_SET);
    struct mainHeader header;
    fread(&header, sizeof(header), 1, track);
    fwrite(&header, sizeof(header), 1, output);

    long long valueSize = strlen(frameValue) + 1; // Include null terminator
    long long fileSize = getSizeTag(header.size);
    struct frameHeader headerFrame;

    long long slide = 0;
    while (slide < fileSize) {
        fread(&headerFrame, sizeof(headerFrame), 1, track);
        int frameSize = getSizeFrame(headerFrame.size);

        if (strcmp(headerFrame.frameID, frameName) != 0 && frameSize + slide <= fileSize) {
            fwrite(&headerFrame, sizeof(headerFrame), 1, output);
            char *data = malloc(frameSize);
            fread(data, frameSize, 1, track);
            fwrite(data, frameSize, 1, output);
            free(data);
        } else if (strcmp(headerFrame.frameID, frameName) == 0) {
            int initialFrameSize = frameSize;
            long long curFileSize = getSizeTag(header.size) - frameSize + valueSize;
            setSizeFrame(header.size, curFileSize);

            long long curSlide = ftell(output);
            fseek(output, 6, SEEK_SET);
            fwrite(&header.size, sizeof(header.size), 1, output);
            fseek(output, curSlide, SEEK_SET);

            setSizeFrame(headerFrame.size, valueSize);
            fwrite(&headerFrame, sizeof(headerFrame), 1, output);
            fwrite(frameValue, valueSize, 1, output);

            fseek(track, initialFrameSize - sizeof(headerFrame), SEEK_CUR);
        }
        slide += sizeof(headerFrame) + frameSize;
    }

    int part;
    while ((part = fgetc(track)) != EOF) {
        fputc(part, output);
    }
    fseek(track, 0, SEEK_SET);
    fseek(output, 0, SEEK_SET);
    while ((part = fgetc(output)) != EOF) {
        fputc(part, track);
    }

    fclose(track);
    fclose(output);

    printf("\nFrames information successfully changed.\n");
}

void printFrames(FILE *track){ // TIT2 TPE1 TALB TYER TCON
    fseek(track, 0, SEEK_SET); // Устанавливаем каретку (указатель) на начало файла
    struct mainHeader header; // Создаем экземпляр структуры
    // struct mainHeader{
    //    char mainID[3];
    //    char version[2];
    //    char flags[1];
    //    unsigned char size[4];
    //};
    fread(&header, sizeof(header), 1, track);
    fseek(track, sizeof(header), SEEK_SET);
    long long tagsSize = getSizeTag(header.size);
    long long slide = 0;
    // 500
    struct frameHeader frameHeader;
    // struct frameHeader{
    //    char frameID[4]; TIT2
    //    unsigned char size[4]; 20
    //    char flags[2]; 01
    //};
    while (slide <= tagsSize){
        fread(&frameHeader, sizeof(frameHeader), 1, track);
        long long frameSize = getSizeFrame(frameHeader.size);
        if (frameHeader.frameID[0] == 'T'){ // TIT2 TPE1 TALB TYER TCON
            printf("%s ", frameHeader.frameID);
            char buffer[frameSize]; // char buffer[4];
            size_t bytesRead = fread(buffer, 1, sizeof(buffer), track);
            if (bytesRead == 0) {
                break;
            }
            fwrite(buffer, 1, bytesRead, stdout);
        }
        printf("\n");
        slide += sizeof(frameHeader) + frameSize;
    }
    printf("\n");
}

void printFrame(FILE *track, char *frame){
    fseek(track, 0, SEEK_SET);
    struct mainHeader header;
    fread(&header, sizeof(header), 1, track);
    fseek(track, sizeof(header), SEEK_SET);
    long long tagsSize = getSizeTag(header.size);
    long long slide = 0;
    struct frameHeader frameHeader;
    while (slide <= tagsSize){
        fread(&frameHeader, sizeof(frameHeader), 1, track);
        long long frameSize = getSizeFrame(frameHeader.size);
        slide += sizeof(frameHeader) + frameSize;
        if (frameHeader.frameID[0] == frame[0] && frameHeader.frameID[3] == frame[3]) { // search frame
            printf("%s ", frameHeader.frameID);
            char buffer[frameSize];
            size_t bytesRead = fread(buffer, 1, sizeof(buffer), track);
            if (bytesRead == 0) {
                break;
            }
            fwrite(buffer, 1, bytesRead, stdout);
        }
    }
    printf("\n");
}