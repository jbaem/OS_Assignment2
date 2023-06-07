#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_NUM 64
#define BLOCK_SIZE 512
#define INODE_SIZE 16
#define ENTRY_SIZE 4

#define I_BMAP_BASE 512
#define D_BMAP_BASE 512 + 256
#define I_BLOCK_BASE 1024
#define D_BLOCK_BASE 2048

void InitDataStorage();
void InitRootDirectory();

void FileSystem(char*);

void ReadFile(char*, int);
void WriteFile(char*, int);
void DeleteFile(char*);

int CreateFile(char*);
int AllocateNewBlock();
int FindEmptySpace();
int FindFile(char*);



typedef struct {
	unsigned int fsize;
	unsigned int blocks;
	unsigned int dptr;
	unsigned int iptr;
} Inode;

typedef struct {
	unsigned char inum;
	char fileName[3];
} DirectoryEntry;

unsigned char* data_storage;

int main(int argc, char* argv[]) {
//	if (argc != 2) {
//		printf("Error: the number of arguments.\n");
//		return 1;
//	}
	
	InitDataStorage();
	InitRootDirectory();

	FileSystem("input_file.txt");
	
	free(data_storage);

	return 0;
}
/* super, i=0,1 */
void InitDataStorage() {
	/* allocate storage */
	data_storage = (char*)calloc(BLOCK_NUM, BLOCK_SIZE);
	/* super -> empty */

	/* ibmap 0,1 -> use */
	*(data_storage + I_BMAP_BASE) = 192; //1100 0000
	return;
	/* inode 0,1 -> empty */

}
/* i=2, d=0 */
void InitRootDirectory() {
	/* ibmap 2 -> use */
	*(data_storage + I_BMAP_BASE) += 32; //0010 0000
	/* inode 2 -> update */
	Inode* root_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	root_inode->fsize = 4 * 61;
	root_inode->blocks = 1;
	root_inode->dptr = 0;
	/* dbmap 0 -> use */
	*(data_storage + D_BMAP_BASE + root_inode->dptr) = 128;
	/* dblock 0 -> empty(no entry) */

}
/* open -> read lines <-> command -> EOF -> close */
void FileSystem(char* input_file_name) {
	/* file open */
	FILE* input_file = fopen(input_file_name, "r");
	if (input_file == NULL) {
		printf("Error: file open failed.\n");
		return;
	}
	
	/* read file line by line */
	char file_name[3];
	char io_command;
	unsigned int word_size;
	int rc;
	while ((rc = fscanf(input_file, "%s %c", file_name, &io_command)) != EOF) {
		
		switch (io_command) {
		case 'w': /* write */
			fscanf(input_file, "%d", &word_size);
			WriteFile(file_name, word_size);
			break;
		case 'r': /* read */
			fscanf(input_file, "%d", &word_size);
			ReadFile(file_name, word_size);
			break;
		case 'd': /* delete */
			DeleteFile(file_name);
			break;
		default: /* else */
			printf("");
		}
	}
	
	/* EOF -> all bits in hexadecimal */
	fclose(input_file);

	for (int i = 0; i < BLOCK_NUM * BLOCK_SIZE; ++i) {
		printf("%.2x ", *(data_storage + i));
		if (i % BLOCK_NUM == BLOCK_NUM - 1) printf("\n\n");
	}
	printf("\n");
	
	free(data_storage);
	return;
}
/* inum -> */
void WriteFile(char* file_name, int word_size) {

	/* search file */
	int curr_inode_index = FindFile(file_name);
	/* file -> not exist */
	if (curr_inode_index == -1) {
		curr_inode_index = CreateFile(file_name);
		/* no space */
		if (curr_inode_index == -1) {
			return;
		}
	}

	/* write nothing */
	if (word_size == 0) {
		return;
	}

	char letter = file_name[0];
	/* file -> exist */
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + curr_inode_index);

	unsigned int curr_block_fsize = curr_inode->fsize - BLOCK_SIZE * curr_inode->blocks;

	unsigned char* write_index;
	if (curr_inode->blocks == 1) {
		/* dptr */
		write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr + curr_block_fsize);
	}
	else {
		/* iptr */
		int block_index = *((int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr) + curr_inode->blocks - 1);
		if (block_index == 0) {
			block_index = AllocateNewBlock();
			if (block_index == -1) {
				/* no space */
				return;
			}
		}
		
		write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * block_index);
	}
	while (curr_inode->fsize % BLOCK_SIZE + 1 < BLOCK_SIZE && word_size > 0) {
		*(write_index) = letter;
		write_index++;
		(curr_inode->fsize)++;
		word_size--;
	}

	WriteFile(file_name, word_size);
	return;
}
/**/
void ReadFile(char* file_name, unsigned int word_size) {
	//비트맵 접근
	int curr_ibmap_index = FindFile(file_name);
	//파일 없음
	if (curr_ibmap_index == -1) {
		printf("No such file");
		return;
	}
	//아이 블럭 접근
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + curr_ibmap_index);

	//데이터 블러 접근
	int curr_fsize = curr_inode->fsize;
	char* curr_block = data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr;

	//한개의 블럭. dptr로 접근
	for (int i = 0; i < BLOCK_SIZE; ++i) {
		if (word_size <= 0 || curr_fsize <= 0) {
			return;
		}
		printf("%c", *(curr_block + i));
		word_size--;
		curr_fsize--;
	}
	if (curr_inode->blocks == 1) {
		return; //다른 블럭이 할당되지 않음
	}

	//여러 개의 블럭. iptr로 접근, word_size, curr_fsize 가 아직 남아 있음.
	int* pre_block = (int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr);
	for (int i = 0; i < BLOCK_SIZE / 4; ++i) {
		curr_block = data_storage + D_BLOCK_BASE + BLOCK_SIZE * (*(pre_block + i));

		for (int i = 0; i < BLOCK_SIZE; ++i) {
			if (word_size <= 0 || curr_fsize <= 0) {
				return;
			}
			printf("%c", *(curr_block + i));
			word_size--;
			curr_fsize--;
		}
	}
	return;
}

/**/
void DeleteFile(char* file_name) {
	
	int curr_ibmap_index = FindFile(file_name);
	if (curr_ibmap_index == -1) {
		printf("No such file\n");
		return;
	}
}
/* */
int CreateFile(char* file_name) {
	//공간이 없다
	//반환값이 엔트리 인덱스
	int entry_index = FindEmptySpace();
	if (entry_index == -1) {
		printf("No space\n");
		return -1;
	}

	//공간이 있다
	//ibmap 인덱스를 찾아
	int create_inode_index = 0;
	for (int i = 0; BLOCK_SIZE / 2; ++i) {
		if ((char*)(data_storage + I_BMAP_BASE + i) == 0x01) continue;
		create_inode_index = i;
		*(data_storage + I_BMAP_BASE + i) = 0x01;
		break;
	}

	//dbmap 인덱스를 찾아
	int create_block_index = 0;
	for (int i = 0; BLOCK_SIZE / 2; ++i) {
		if (*(data_storage + D_BMAP_BASE + i) == 0x01) continue;
		create_block_index = i;
		*(data_storage + D_BMAP_BASE + i) = 0x01;
		break;
	}

	//inode를 연결
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + create_inode_index);
	curr_inode->blocks = 0;
	curr_inode->dptr = create_inode_index;

	return create_inode_index;
}
int AllocateNewBlock() {

}

/* find empty entry & return entry index */
int FindEmptySpace() {
	/* root_inode */
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	/* root_inode.dptr(==0) ->  root_dblock(table) */
	DirectoryEntry* root_entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr));

	/* search empty entry */
	for (int i = 0; i < BLOCK_SIZE / ENTRY_SIZE; ++i) {
		/* not used entry */
		if ((root_entry + i)->inum == 0) {
			return i;
		}
	}
	/* table is full */
	return -1;
}
/* find inum(ibmap index) by file name */
int FindFile(char* file_name) {
	/* root_inode */
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	/* root_inode.dptr(==0) ->  root_dblock(table) */
	DirectoryEntry* root_entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr));

	/* search file by file name */
	for (int i = 0; i < BLOCK_SIZE / ENTRY_SIZE; ++i) {
		/* not used entry */
		if ((root_entry + i)->inum == 0) continue;
		/* same file name */
		if (strcmp((root_entry + i)->fileName, file_name) == 0) {
			return (root_entry + i)->inum;
		}
	}
	//해당 파일 없음
	return -1;
}