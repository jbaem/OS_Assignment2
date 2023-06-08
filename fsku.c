#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_NUM 64
#define BLOCK_SIZE 512
#define INODE_SIZE 16
#define ENTRY_SIZE 4

#define I_BMAP_BASE 512
#define D_BMAP_BASE (512 + 256)
#define I_BLOCK_BASE 1024
#define D_BLOCK_BASE 2048

void InitDataStorage();
void InitRootDirectory();

void FileSystem(char* input_file_name);

void ReadFile(char* file_name, int word_size);
void WriteFile(char* file_name, int word_size);
void DeleteFile(char* file_name);

int CreateFile(char* file_name);
int AllocateNewBlock();
int FindEmptySpace();
int FindFile(char*);
int FindDBmap();
void SetBitMap(int base, int index, char flag);

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
//TODO: change comments
//	if (argc != 2) {
//		printf("Error: the number of arguments.\n");
//		return 1;
//	}

	InitDataStorage();
	InitRootDirectory();

	FileSystem("input_file.txt"); //TODO: change argument

	free(data_storage);

	return 0;
}
/* super, i=0,1 */
void InitDataStorage() {
	/* allocate storage */
	data_storage = calloc(BLOCK_NUM, BLOCK_SIZE);
	/* super -> empty */
	if (data_storage != NULL) {
		/* ibmap 0,1 -> use */
		*(data_storage + I_BMAP_BASE) = 192; //1100 0000
	}
	return;
	/* inode 0,1 -> empty(not use) */

}
/* i=2, d=0 */
void InitRootDirectory() {
	/* ibmap 2 -> use */
	*(data_storage + I_BMAP_BASE) += 32; //0010 0000
	/* inode 2 -> update */
	Inode* root_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	root_inode->fsize = 4 * 61; // 1111 0100
	root_inode->blocks = 1; //0000 0001
	root_inode->dptr = 0; //0000 0000
	/* dbmap 0 -> use */
	*(data_storage + D_BMAP_BASE + root_inode->dptr) = 128;
	/* dblock 0 -> empty(no entry) -> will be used */

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
	int line = 0;
	while ((rc = fscanf(input_file, "%s %c", file_name, &io_command)) != EOF) {
		printf("%d: %c\n", ++line, io_command);
		switch (io_command) {
		case 'w': /* write */
			fscanf(input_file, "%d", &word_size);
			WriteFile(file_name, word_size);
			break;
		case 'r': /* read */
			fscanf(input_file, "%d", &word_size);
			ReadFile(file_name, word_size);
			printf("\n");
			break;
		case 'd': /* delete */
			DeleteFile(file_name);
			break;
		default: /* else */
			printf("Error: invalid command\n");
		}
	}

	/* EOF -> all bits in hexadecimal */
	fclose(input_file);
	int temp = 0; //TODO: erase
	for (int i = 0; i < BLOCK_NUM * BLOCK_SIZE; ++i) {
		if (i == 0) printf("<Super>\n");							//TODO: erase
		if (i == I_BMAP_BASE) printf("<i-bmap>\n");				//TODO: erase
		if (i == D_BMAP_BASE) printf("<d-bmap>\n");				//TODO: erase
		if (i == I_BLOCK_BASE) printf("<i-block>\n");			//TODO: erase
		if (i == D_BLOCK_BASE) printf("<d-block>\n");			//TODO: erase
		if (i % BLOCK_SIZE == 0) printf(">%d th block\n", i / BLOCK_SIZE);
		if (i % 8 == 0) printf("\n %d : ", ++temp);				//TODO: erase
		printf("%02x ", *(data_storage + i));
		if (i % BLOCK_NUM == BLOCK_NUM - 1) printf("\n\n");		//TODO: erase
	}
	printf("\n");

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
			printf("No space\n");
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

	unsigned char* write_index;

	while (word_size > 0) {
		if (curr_inode->fsize < BLOCK_SIZE) {
			/* dptr */
			write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr + curr_inode->fsize);
		}
		else {
			/* iptr */
			int block_index = *((int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr) + curr_inode->blocks - 2);
			if (block_index == 0) {
				block_index = AllocateNewBlock();
				printf("\n%d\n", block_index);
				if (block_index == -1) {
					/* no space */
					printf("No space\n");
					return;
				}
				*((int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr) + curr_inode->blocks - 2) = block_index;
				curr_inode->blocks += 1;
			}
			write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * block_index);
		}
		int fsize = curr_inode->fsize % BLOCK_SIZE;

		while (fsize + 1 <= BLOCK_SIZE && word_size > 0) {
			*(write_index) = letter;
			write_index++;
			(curr_inode->fsize)++;
			word_size--;
			fsize++;
		}
	}
	printf("\nWF\n");
	return;
}
void ReadFile(char* file_name, int word_size) {
	int curr_ibmap_index = FindFile(file_name);
	if (curr_ibmap_index == -1) {
		printf("No such file.");
		return;
	}

	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + curr_ibmap_index);

	unsigned int curr_fsize = curr_inode->fsize;
	unsigned char* curr_block = data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr;

	// Read from a single block pointed by dptr
	for (int i = 0; i < BLOCK_SIZE; ++i) {
		if (word_size <= 0 || curr_fsize <= 0) {
			return;
		}
		printf("%c", (char)curr_block[i]);
		word_size--;
		curr_fsize--;
	}

	// Read from multiple blocks pointed by iptr
	int* pre_block = (int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr);
	for (int i = 0; i < curr_inode->blocks - 2; ++i) {
		curr_block = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * (pre_block + i)[0]);

		for (int i = 0; i < BLOCK_SIZE; ++i) {
			if (word_size <= 0 || curr_fsize <= 0) {
				return;
			}
			printf("%c", (char)curr_block[i]); //TODO: update
			word_size--;
			curr_fsize--;
		}
	}
	printf("\nRF\n");
	return;
}

void DeleteFile(char* file_name) {
	int curr_ibmap_index = FindFile(file_name);
	if (curr_ibmap_index == -1) {
		printf("No such file.\n");
		return;
	}
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + curr_ibmap_index);
	if (curr_inode->iptr != 0) {
		int* curr_iptr = (int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr);
		for (int i = 0; i < curr_inode->blocks - 2; ++i) {
			int dbmap_index = *(curr_iptr + i);
			if (dbmap_index != 0) {
				SetBitMap(D_BMAP_BASE, dbmap_index, 'n');
			}
		}
		//dbmap에서 iptr 값 삭제
		SetBitMap(D_BMAP_BASE, curr_inode->iptr, 'n');
	}
	//dbmap에서 dptr 값 삭제
	SetBitMap(D_BMAP_BASE, curr_inode->dptr, 'n');
	
	//ibmap에서 inum에 해당 하는 값 삭제
	DirectoryEntry* dir_entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE) + curr_ibmap_index - 3);
	SetBitMap(I_BMAP_BASE, dir_entry->inum, 'n');
	
	//inum = 0 으로 만들어
	dir_entry->inum = 0;
	printf("\nDF\n");
	return;
}
/* */
void SetBitMap(int base, int index, char flag) {
	int byte_index = index / 8;
	int bit_index = index % 8;
	if (flag == 'a') {
		*(data_storage + base + byte_index) |= (1 << (7 - bit_index));
	}
	else {
		*(data_storage + base + byte_index) &= ~(1 << (7 - bit_index));
	}
}

int CreateFile(char* file_name) {
	/* find empty entry */
	int empty_entry_index = FindEmptySpace();
	if (empty_entry_index == -1) {
		/* no space */
		return -1;
	}

	/* update entry */
	DirectoryEntry* entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE) + empty_entry_index);
	entry->inum = empty_entry_index + 3;
	strncpy(entry->fileName, file_name, 3);

	/* allocate inode */
	int inode_index = empty_entry_index + 3;
	Inode* inode = ((Inode*)(data_storage + I_BLOCK_BASE) + inode_index);
	inode->fsize = 0;
	inode->blocks = 2;
	inode->dptr = FindDBmap();
	inode->iptr = AllocateNewBlock();

	if (inode->dptr == -1 || inode->iptr == -1) {
		return -1;
	}
	SetBitMap(I_BMAP_BASE, inode_index, 'a');
	
	return inode_index;
}
/* */
int AllocateNewBlock() {
	/* find empty block */
	int empty_block_index = FindDBmap();
	if (empty_block_index == -1) {
		/* no space */
		return -1;
	}
	/* update dbmap */
	SetBitMap(D_BMAP_BASE, empty_block_index, 'a');

	return empty_block_index;
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

	return -1;
}

int FindDBmap() {
	int dbmap_index = 0;
	unsigned char* curr_dbmap = data_storage + D_BMAP_BASE;
	for (int i = 0; i < 60; ++i) {
		unsigned char curr = *(curr_dbmap + i);

		if (curr != 0xFF) {
			for (int j = 0; j < 8; ++j) {
				if ((curr & (1 << (7 - j))) == 0) {
					*(curr_dbmap + i) = curr | (1 << (7 - j));
					return dbmap_index + j;
				}
			}
		}

		dbmap_index += 8;
	}

	return -1;
}