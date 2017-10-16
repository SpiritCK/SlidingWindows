#ifndef SEGMENT_H
#define SEGMENT_H

char* changeToBinary(char* arr, int size1, int* size2);

unsigned char* changeToArray(unsigned char* msg, int size1, int pad, int* size2);

unsigned char encryptCRC(unsigned char* msg, int size1);

int decryptCRC(unsigned char* msg, int s);

#endif /* CRC_H */
