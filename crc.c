#include<stdio.h>
#include<stdlib.h>

char CRC_8[9] = {1, 0, 0, 0, 0, 0, 1, 1, 1};

char* changeToBinary(char* arr, int size1, int* size2) {
	int i = size1 - 1;
	int j = 1;
	int sum = (size1 - 1)/8 + 1;
	char* result = (char*) malloc(sizeof(char) * sum);
	while (i >= 7) {
		result[sum-j] = arr[i] + 2*arr[i-1] + 4*arr[i-2] + 8*arr[i-3]
				+ 16*arr[i-4] + 32*arr[i-5] + 64*arr[i-6]
				+ 128*arr[i-7];
		i -= 8;
		j++;
	}
	result[sum-j] = 0;
	int exp = 0;
	while (i >= 0) {
		int temp = arr[i];
		if (temp > 0) {
			for (int k = 0; k < exp; k++) {
				temp *= 2;
			}
			result[sum-j] += temp;
		}
		i--;
		exp++;
	}
	size2[0] = sum;
	return result;
}

unsigned char* changeToArray(unsigned char* msg, int size1, int pad, int* size2) {
	int size = size1*8;
	unsigned char* arr = (unsigned char*) malloc(sizeof(char) * (size+pad));
	for (int i = 0; i < size1; i++) {
		int temp = msg[i];
		for (int j = 0; j < 8; j++) {
			arr[i*8+7-j] = temp % 2;
			temp /= 2;
		}
	}
	for (int i = size; i < size+pad; i++) {
		arr[i] = 0;
	}
	size2[0] = size+pad;
	return arr;
}

unsigned char* encryptCRC(unsigned char* msg, int size1, int* size2) {
	unsigned char* arr = changeToArray(msg, size1, 8, size2);
	unsigned char* result = (unsigned char*) malloc(sizeof(char) * size2[0]);
	for (int i = 0; i < size2[0]-8; i++) {
		result[i] = arr[i];
	}
	int i = 0;
	while (i < (size2[0] - 8)) {
		if (arr[i] == 1) {
			for (int j = 0; j <= 8; j++) {
				arr[i+j] ^= CRC_8[j];
			}
		}
		i++;
	}
	for (int i = size2[0]-8; i < size2[0]; i++) {
		result[i] = arr[i];
	}
	return result;
}

int decryptCRC(unsigned char* msg, int s) {
	FILE *fp1 = fopen("hex2.bin", "w");
	FILE *fp2 = fopen("bin2.bin", "w");
	for (int i = 0; i < s; i++) {
		fwrite(&msg[i], 1, sizeof(msg[i]), fp1);
	}
	int* size = (int*) malloc(sizeof(int));
	unsigned char* arr = changeToArray(msg, s, 0, size);
	for (int i = 0; i < size[0]; i++) {
		fwrite(&arr[i], 1, sizeof(arr[i]), fp2);
	}
	unsigned char* result = (unsigned char*) malloc(sizeof(char) * size[0]);
	for (int i = 0; i < size[0]-8; i++) {
		result[i] = arr[i];
	}
	int i = 0;
	while (i < (size[0] - 8)) {
		if (arr[i] == 1) {
			for (int j = 0; j <= 8; j++) {
				arr[i+j] ^= CRC_8[j];
			}
		}
		i++;
	}
	int valid = 1;
	for (int i = size[0]-8; i < size[0]; i++) {
		if (arr[i] == 1) {
			valid = 0;
		}
	}
	fclose(fp1);
	fclose(fp2);
	return valid;
}

int main() {
	int n;
	FILE *fp1 = fopen("hex1.bin", "w");
	FILE *fp2 = fopen("bin1.bin", "w");
	scanf("%d", &n);
	unsigned char* msg = (unsigned char*) malloc(sizeof(char)*n);
	for (int i = 0; i < n; i++) {
		int temp;
		scanf("%x", &temp);
		msg[i] = (unsigned char) temp & 0xff;
	}
	int* size = (int*) malloc(sizeof(int));
	unsigned char* result = encryptCRC(msg, n, size);
	
	for (int i = 0; i < size[0]; i++) {
		fwrite(&result[i], 1, sizeof(result[i]), fp2);
	}
	
	unsigned char* bin = changeToBinary(result, size[0], size);
	
	for (int i = 0; i < size[0]; i++) {
		fwrite(&bin[i], 1, sizeof(bin[i]), fp1);
	}
	
	bin[size[0] - 1] ^= 7;
	bin[size[0] - 2] ^= 1;
	int check = decryptCRC(bin, size[0]);
	printf("%d\n",check);
	
	fclose(fp1);
	fclose(fp2);
	
	return 0;
}
