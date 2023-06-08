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
	if (argc != 2) {
		printf("Error: the number of arguments.\n");
		return 1;
	}

	InitDataStorage();
	InitRootDirectory();

	FileSystem(argv[1]);

	free(data_storage);

	return 0;
}

void InitDataStorage() {
	data_storage = calloc(BLOCK_NUM, BLOCK_SIZE);
	if (data_storage != NULL) {
		//super: 0
		//ibmap: 0 and 1 used
		*(data_storage + I_BMAP_BASE) = 192; //1100 0000
	}
	return;
}

void InitRootDirectory() {
	//ibmap: 2 used
	*(data_storage + I_BMAP_BASE) += 32; //0010 0000
	//inode: 2 fill
	Inode* root_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	root_inode->fsize = 4 * 61; // 1111 0100
	root_inode->blocks = 1; //0000 0001
	root_inode->dptr = 0; //0000 0000
	//dbmap: 0(dptr) used
	*(data_storage + D_BMAP_BASE + root_inode->dptr) = 128; //1000 0000
}

void FileSystem(char* input_file_name) {
	FILE* input_file = fopen(input_file_name, "r");
	if (input_file == NULL) {
		printf("Error: file open failed.\n");
		return;
	}

	char file_name[3];
	char io_command;
	unsigned int word_size;
	int rc;
	int line = 0;
	while ((rc = fscanf(input_file, "%s %c", file_name, &io_command)) != EOF) {
		/*
		int temp = 0; //TODO: erase
		for (int i = 0; i < BLOCK_NUM * BLOCK_SIZE; ++i) {
			if (i == 0) printf("<Super>\n");							//TODO: erase
			if (i == I_BMAP_BASE) printf("<i-bmap>\n");				//TODO: erase
			if (i == D_BMAP_BASE) printf("<d-bmap>\n");				//TODO: erase
			if (i == I_BLOCK_BASE) printf("<i-block>\n");			//TODO: erase
			if (i == D_BLOCK_BASE) printf("<d-block>\n");			//TODO: erase
			if (i % BLOCK_SIZE == 0) printf(">%d th block\n", i / BLOCK_SIZE);
			if (i % 8 == 0) printf("\n %d : ", ++temp);				//TODO: erase
			printf("%c ", *(data_storage + i));
			if (i % BLOCK_NUM == BLOCK_NUM - 1) printf("\n\n");		//TODO: erase
		}
		printf("\n");
		*/
		
		printf("%d: %c\n", ++line, io_command);
		switch (io_command) {
		case 'w':
			fscanf(input_file, "%d", &word_size);
			WriteFile(file_name, word_size);
			break;
		case 'r':
			fscanf(input_file, "%d", &word_size);
			ReadFile(file_name, word_size);
			printf("\n");
			break;
		case 'd':
			DeleteFile(file_name);
			break;
		default:
			printf("Error: invalid command\n");
		}
	}

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

void WriteFile(char* file_name, int word_size) {
	//inum in entry
	int inum = FindFile(file_name);
	if (inum == -1) {
	
		inum = CreateFile(file_name);
		if (inum == -1) {
			printf("No space\n");
			return;
		}
	}

	if (word_size == 0) {
		return;
	}

	char letter = file_name[0];
	//inode for write
	Inode* write_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + inum);
	
	//block for write and 
	unsigned char* write_index;
	while (word_size > 0) {
		//offset
		int fsize = write_inode->fsize % BLOCK_SIZE;
	
		if (write_inode->fsize < BLOCK_SIZE) {
			write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * write_inode->dptr + fsize);
			printf("1\n");
		}
		else if(fsize != 0) {
			int block_index = *((int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * write_inode->iptr) + write_inode->blocks - 3);			
			write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * block_index);
			printf("2\n");
		} 
		else {
			int block_index = AllocateNewBlock();
			if (block_index == -1) {
				printf("No space\n");
				return;
			}
			printf("3\n");	
			*((int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * write_inode->iptr) + write_inode->blocks - 2) = block_index;
			write_inode->blocks += 1;
			
			write_index = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * block_index);
		}
		
		while (fsize + 1 <= BLOCK_SIZE && word_size > 0) {
			*(write_index++) = letter;
			(write_inode->fsize)++;
			word_size--;
			fsize++;
		}
		printf("5\n");
	}

	return;
}

void ReadFile(char* file_name, int word_size) {
	//inum in entry
	int inum = FindFile(file_name);
	if (inum == -1) {
		printf("No such file.");
		return;
	}

	//inode for read
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + inum);
	//amount
	int fsize = curr_inode->fsize;
	//block for read
	unsigned char* read_block = data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr;

	//read dptr
	for (int i = 0; i < BLOCK_SIZE; ++i) {
		if (word_size <= 0 || fsize <= 0) {
			return;
		}
		printf("%c", (char)read_block[i]);
		word_size--;
		fsize--;
	}

	//read iptr
	int* iptr_block = (int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr);
	for (int i = 0; i < curr_inode->blocks - 2; ++i) {
		//block for read
		read_block = (unsigned char*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * *(iptr_block + i));

		for (int i = 0; i < BLOCK_SIZE; ++i) {
			if (word_size <= 0 || fsize <= 0) {
				return;
			}
			printf("%c", (char)read_block[i]);
			word_size--;
			fsize--;
		}
	}

	return;
}

void DeleteFile(char* file_name) {
	//inum in entry
	int inum = FindFile(file_name);
	if (inum == -1) {
		printf("No such file.\n");
		return;
	}
	//inode for delete
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + inum);
	
	//delete iptr index
	int* curr_iptr = (int*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->iptr);
	for (int i = 0; i < curr_inode->blocks - 2; ++i) {
		int dbmap_index = *(curr_iptr + i);
		if (dbmap_index != 0) {
			SetBitMap(D_BMAP_BASE, dbmap_index, 'n');
		}
	}
	//delete iptr
	SetBitMap(D_BMAP_BASE, curr_inode->iptr, 'n');

	//delete dptr
	SetBitMap(D_BMAP_BASE, curr_inode->dptr, 'n');
	
	//delete inum in ibmap
	DirectoryEntry* dir_entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE) + inum - 3);
	SetBitMap(I_BMAP_BASE, dir_entry->inum, 'n');
	
	//inum = 0
	dir_entry->inum = 0;
	
	return;
}

void SetBitMap(int base, int index, char flag) {
	int byte_index = index / 8;
	int bit_index = index % 8;
	//allocate
	if (flag == 'a') {
		*(data_storage + base + byte_index) |= (1 << (7 - bit_index));
	}
	//remove
	else {
		*(data_storage + base + byte_index) &= ~(1 << (7 - bit_index));
	}
}

int CreateFile(char* file_name) {
	//empty index
	int empty_entry_index = FindEmptySpace();
	if (empty_entry_index == -1) {
		return -1;
	}
	//set entry
	DirectoryEntry* entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE) + empty_entry_index);
	entry->inum = empty_entry_index + 3;
	strncpy(entry->fileName, file_name, 3);

	//set inode
	int inode_index = entry->inum;
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

int AllocateNewBlock() {
	//empty dbmap index
	int empty_block_index = FindDBmap();
	if (empty_block_index == -1) {
		return -1;
	}
	
	SetBitMap(D_BMAP_BASE, empty_block_index, 'a');

	return empty_block_index;
}

int FindEmptySpace() {
	//inode root
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	//block root
	DirectoryEntry* root_entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr));

	for (int i = 0; i < BLOCK_SIZE / ENTRY_SIZE; ++i) {
		if ((root_entry + i)->inum == 0) {
			return i;
		}
	}
	return -1;
}

int FindFile(char* file_name) {
	//inode root
	Inode* curr_inode = ((Inode*)(data_storage + I_BLOCK_BASE) + 2);
	//block root
	DirectoryEntry* root_entry = ((DirectoryEntry*)(data_storage + D_BLOCK_BASE + BLOCK_SIZE * curr_inode->dptr));

	for (int i = 0; i < BLOCK_SIZE / ENTRY_SIZE; ++i) {
		if ((root_entry + i)->inum == 0) continue;
		if (strcmp((root_entry + i)->fileName, file_name) == 0) {
			return (root_entry + i)->inum;
		}
	}

	return -1;
}

int FindDBmap() {
	//byte
	int dbmap_index = 0;
	//base
	unsigned char* curr_dbmap = data_storage + D_BMAP_BASE;
	for (int i = 0; i < 60; ++i) {
		unsigned char curr = *(curr_dbmap + i);

		if (curr != 0xFF) {
			//bit
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