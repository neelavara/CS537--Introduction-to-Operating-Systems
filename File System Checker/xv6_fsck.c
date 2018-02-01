#include<assert.h>
#include<stdio.h>
#include<stdbool.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/mman.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>

#define ROOTINUM 1
#define BLOCKSIZE 512
#define NDIRECT 12

#define DIRSIZE 14

#define NINODES 200 
#define SIZE 1024 // Super block

//dinode is a structure information about the inode
#define IPB (BLOCKSIZE / sizeof(struct dinode))

//void execute(struct dinode);

void error(int id){
	switch(id){
		case 0: fprintf(stderr,"image not found.\n");
			break;
		case 1: fprintf(stderr,"Usage: xv6_fsck <file_system_image>.\n");
			break;
                case 2: fprintf(stderr,"ERROR: root directory does not exist.\n");
                        break;
                case 3: fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
                        break;
                case 4: fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
                        break;
                case 5: fprintf(stderr,"ERROR: parent directory mismatch.\n");
                        break;
                case 6: fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
                        break;
                case 7: fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
                        break;
                case 8: fprintf(stderr,"ERROR: bad reference count for file.\n");
                        break;
                case 9: fprintf(stderr,"ERROR: bad inode.\n");
                        break;
                case 10:fprintf(stderr,"ERROR: bad direct address in inode.\n");
                        break;
                case 11:fprintf(stderr,"ERROR: direct address used more than once.\n");
			break;
                case 12:fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
                        break;
                case 13:fprintf(stderr,"ERROR: directory not properly formatted.\n");
                        break;
		case 14:fprintf(stderr,"ERROR: bad indirect address in inode.\n");
			break;
		case 15:fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
			break;
		case 16:fprintf(stderr,"ERROR: indirect address used more than once.\n");
			break;
               
	}
}

struct superblock{
  uint size;
  uint nblocks;
  uint ninodes;
};

struct dinode{
  short type;
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};

struct dirent{
  ushort inum;
  char name[DIRSIZE];
};

void *block;
struct superblock *sb;
char *bitmap;
int dblock;

//Track
int dinode[NINODES]; // for directory
int reference_to_parent[NINODES];
int ref_back[NINODES];
int references[NINODES];
int real_references[NINODES];
int dbitmap[NINODES];

bool root = false;

void execute(struct dinode *d){
        int i, j=0, cinum;
        struct dirent *dir_entry;
        uint *indirectBlock;
        int direntry = BLOCKSIZE/sizeof(struct dirent);
        if(d->type < 0 || d->type > 3){
                //error(9);
		fprintf(stderr,"ERROR: bad inode.\n");
                exit(1);
        }
        if(d->type == 0)
             return;
        int buse, bitval;
        int indirect_blocks = BLOCKSIZE/sizeof(uint);
        // Direct Block
          for(i=0;i<=NDIRECT;i++){
		if(d->addrs[i]!=0 && (d->addrs[i] < dblock || d->addrs[i] > 1023)){
			//error(10);
			fprintf(stderr,"ERROR: bad direct address in inode.\n");
			exit(1);
		}
                buse = d->addrs[i];
                bitval = dbitmap[buse];
                if(buse > 0){
			dbitmap[buse]++;
                        if(dbitmap[buse] > 2){
				//error(11);
				fprintf(stderr,"ERROR: direct address used more than once.\n");
				exit(1);
			}
			if(bitval == 0){
				error(12);
				exit(1);
			}
		}

	  // Directory Check
	        if(d->type == 1){
		    dir_entry = (struct dirent*)(block + (buse * BLOCKSIZE));
		    if(i==0 || strcmp((dir_entry+1)->name,"..")){
			 	error(13);
				exit(1);
		    }
                    dinode[dir_entry->inum] = 1;
                    cinum = dir_entry->inum;
		    reference_to_parent[cinum] = (dir_entry+1)->inum;
            
                    if(dir_entry->inum == 1 && (dir_entry+1)->inum ==1 && root == false)
			root = true;
                    j++;
		    j++;
		    dir_entry = dir_entry+2;
		    if(i<NDIRECT){
			while(j<direntry){
			   if(dir_entry->inum!=0){
				references[dir_entry->inum]++;
				ref_back[dir_entry->inum] = cinum;
			   }
			   dir_entry++;
			   j++;
			}
		    }	       
		}
	  }

	// Indirect Blocks
	buse = d->addrs[NDIRECT];
	indirectBlock = (uint*)(block + (buse * BLOCKSIZE));
 	for(i=0;i<indirect_blocks;i++){
		if(*indirectBlock !=0 && (*indirectBlock < dblock || *indirectBlock > 1023)){
			error(14);
			exit(1);
		}
		if(*indirectBlock > 0 && dbitmap[*indirectBlock] == 0){
			error(15);
			exit(1);
		}
		if(*indirectBlock > 0){
			dbitmap[*indirectBlock]++;
		}
		if(dbitmap[*indirectBlock] > 2){
			error(16);
			exit(1);
		}

		// Directory check
		if(d->type == 1){
			dir_entry = (struct dirent*)(block + (*indirectBlock * BLOCKSIZE));
			for(j=0;j<direntry;j++){
				if(dir_entry->inum != 0){
					references[dir_entry->inum]++;
					ref_back[dir_entry->inum] = cinum;
				}
			dir_entry++;
			}
		}
		indirectBlock++;
	}
	return;
}


int main(int argc, char *argv[]){
	int num = argc;
        int i, j, inum, iblocks;
	if(num!=2){
		//error(1);
		fprintf(stderr,"Usage: xv6_fsck <file_system_image>.\n");
		exit(1);
	}
//	printf("Hello");
	//****start of setup bufffers
	int fd = open(argv[1], O_RDONLY); 
        if(fd != 0){
	//      fprintf(stderr,"image not found.\n");
	//	error(0);
		exit(0);
	}
	int rc;
        struct stat sbuf;
	rc = fstat(fd, &sbuf); // Get the size
        assert(rc == 0);
        //if(rc != 0) exit(0); //Changed
        block = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        assert(block!=MAP_FAILED);
	//if(block == MAP_FAILED) exit(0); //Changed
        // As per video
        sb = (struct superblock*)(block + BLOCKSIZE); // first block is unused
        uint bit_block = sb->ninodes / IPB + 3; // 3rd block
        bitmap = (char *) (block + (bit_block * BLOCKSIZE)); // bitmap
        dblock = bit_block + (sb->size/ (BLOCKSIZE * 8) + 1); // Data Block
        //****end of setup buffers
   
        //****start of process inode table
	//Processing the inode block
	        
	inum = 0;
        iblocks = sb->ninodes / IPB;
        //Check if Data Block is used or not
        for(i=dblock;i<sb->size;i++){
	    if((bitmap[i/8] & (0x1 << (i%8))) > 0) {
			dbitmap[i] = 1;
		}
            else
                        dbitmap[i] = 0;
	}

	ref_back[ROOTINUM] = 1;
        references[ROOTINUM] = 1;
        
        for(i=2; i<iblocks+2 ; i++){
	      struct dinode *d = (struct dinode*)(block + (i*BLOCKSIZE));
              for(j=0;j<IPB;j++){
		 execute(d);
		 if(d->type != 0){
			if(d->nlink > 1){
			   real_references[inum] = d->nlink;
			}
			else
			   real_references[inum] = 1;
                 }
                 if(inum == 1 && root == false){
			//error(2);
			fprintf(stderr,"ERROR: root directory does not exist.\n");
                        exit(1);
		 }
                inum++;
		d++;
	      }
	}
       //****end of start of process inode table

       //****start of post processing
	//check biotmaps
	for(i=dblock;i<dblock+sb->nblocks + 1;i++){
		if(dbitmap[i] == 1){
			//error(3);
			fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
			exit(1);
		}
	}
        for(i=0;i<sb->ninodes;i++){
		if(dinode[i] == 1){
			if(references[i] > 1 || real_references[i] > 1){
				//error(4);
				fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
				exit(1);
			}
                        if(reference_to_parent[i] != ref_back[i]){
				//error(5);
				fprintf(stderr,"ERROR: parent directory mismatch.\n");
				exit(1);
			}
		}
		if(references[i] == 0 && real_references[i] > 0){
			//error(6);
			fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
			exit(1);
		}
                //change if u can
                else if(references[i] > 0 && real_references[i] == 0){
			//error(7);
			fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
			exit(1);
                }
                if(dinode[i] == 0 && references[i] != real_references[i]){
			//error(8);
			fprintf(stderr,"ERROR: bad reference count for file.\n");
			exit(1);
		}
	}	
      //****end of post processing
	return 0;

}

/*void execute(struct dinode *d){
	if(d->type < 0 || d->type > 3){
		error(9);
		exit(1);
	}
        if(d->type == 0)
             return;
}*/
