#include "dils.h"
#include <fcntl.h>

using namespace std;

int main(int argc, char **argv)
{
  /* declare a buffer that is the same size as a filesystem block */
  char raw_superblock[128];

  /* Check command line args, should receive disk image and optional
     -l for "long" listing. */
  if(!(argc == 2 || (argc == 3 && strstr(argv[2], "-l") )))
  {
    cout << "Usage \n  ./dils <disk.img>\n  ./dils <disk.img> -l\n";
    exit(1);
  }

  /* Create a pointer to the buffer so that we can treat it as a
     superblock struct. The superblock struct is smaller than a block,
     so we have to do it this way so that the buffer that we read
     blocks into is big enough to hold a complete block. Otherwise the
     driver_read function will overwrite something that should not be
     overwritten. */
  sfs_superblock *super = (sfs_superblock *)raw_superblock;

  /* open the disk image and get it ready to read/write blocks */
  driver_attach_disk_image(argv[1], 128);
  
  /* search the first 10 blocks for the superblock */
  for(int i=0; i<10; i++)
  {
    driver_read(super,i);

    /* is it the filesystem superblock? */
    if(super->fsmagic == VMLARIX_SFS_MAGIC && !strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
    {
      printf("superblock found at block %d!\n", i);
      if(argc == 2)
        ShortListing(super);
      else
        LongListing(super);
      i=20;
    }
    
    /* Give up after 10 blocks checked. */
    if(i == 9)
      printf("superblock is not found in first 10 blocks.\nI quit!\n");
  }
  /* close the disk image */
  driver_detach_disk_image();

  return 0;
}


void LongListing(sfs_superblock *super)
{
  char raw_block[2][128];
  sfs_inode *inode = (sfs_inode*)raw_block[0]; // Grabs two inodes from block
  sfs_dirent *dir = (sfs_dirent *)raw_block[1]; // Grab 4 dirs from block, no longer used, but removing causes seg fault?!?
  char f_name[28];
  char r_dir[3] = "..";

  // int fo = open("rd2.out", O_RDWR);
  // dup2(fo,1);

  /* Print all Inodes. 2 inodes per block, check for odd number  */
  driver_read(inode, super->inodes);
  PrintInodeLongList(inode[0], super, 0,f_name);
  PrintInodeLongList(inode[0], super, 0,r_dir);
  PrintInodeLongList(inode[1], super, 1,f_name);
  for(int i=1; i<super->num_inode_blocks - super->inodes_free/2-1; i++)
  {
    driver_read(inode, super->inodes+i);
    PrintInodeLongList(inode[0], super, i*2,f_name);
    PrintInodeLongList(inode[1], super, i*2+1,f_name);
  }
  driver_read(inode, super->inodes + super->num_inode_blocks - super->inodes_free/2 - 1 );
  if((super->num_inodes - super->inodes_free) % 2 != 1)
  {
    PrintInodeLongList(inode[0], super, super->num_inodes - super->inodes_free-2,f_name);
    PrintInodeLongList(inode[1], super, super->num_inodes - super->inodes_free-1,f_name);
  }
  else
    PrintInodeLongList(inode[0], super, super->num_inodes - super->inodes_free-1,f_name);
}

void PrintInodeLongList(sfs_inode inode, sfs_superblock *super, uint32_t inode_num, char* f_name)
{
  string perms = "drwxrwxrwx";
  uint16_t mask = 0b100000000;
  time_t t = (time_t)inode.atime;
  string atime = (string)ctime(&t);
  
  if(strcmp(f_name, ".."))
    GetFileName(f_name, inode_num, super, &super->rootdir);

  if(inode.type == FT_DIR)
    cout << "d";
  else 
    cout << "-";

  for(int i=1; i<10; i++)
  {
    if((mask & inode.perm) == 0)
      cout << '-';
    else
      cout << perms[i];
    mask = mask >> 1;
  }
  cout << " " << right << unsigned(inode.refcount);
  cout << setw(7) << inode.owner;
  cout << setw(7) << inode.group;
  cout << setw(8) << inode.size;
  cout << " " << atime.substr(4,12) << atime.substr(19,5);
  cout << " " << left << f_name << endl;
}


void ShortListing(sfs_superblock *super)
{
  uint32_t start_block=super->rootdir;
  char fname[28];

  // Loop for number of used inodes
  cout << ".\n..\n";
  for(int i=1; i<super->num_inodes-super->inodes_free; i++)
  {
    // Read a block into 4 sfs_dirents 
    GetFileName(fname, i, super, &start_block);
    cout << fname << endl;
  }
}

/* Very slow and inefficient method of finding file names. I'm sure there is a better way to do this
   But my brain isn't working... so I guess it is what it is. */
void GetFileName(char* fname, uint32_t inode, sfs_superblock *super, uint32_t *start_block)
{
  sfs_dirent dirents[4];
  strcpy(fname, "nada");
  for(uint32_t i=*start_block; i<super->num_blocks; i++)
  {
    // Read a block into 4 sfs_dirents 
    ReadDirectories(i, dirents);
    for(int j=0; j<4; j++)
    {
      if(dirents[j].inode == inode)
      {
        // fname = dirents[j].name;
        strcpy(fname, dirents[j].name);
        return;
      }
    }
  }
}

void ReadDirectories(uint32_t block, sfs_dirent* dirents)
{
  char raw_block[128];
  int j=0, k=0;
  
  // Read a block into 4 sfs_dirents 
  for(int i=0; i<4; i++)
  {
    driver_read(raw_block, block);//+j);
    for(int j=0; j<28; j++)
      dirents[i].name[j] = raw_block[i*32+j];
    dirents[i].inode = raw_block[i*32+31]<<24 | raw_block[i*32+30] << 16 | raw_block[i*32+29] << 8 | raw_block[i*32+28];
    // PrintDir(dirents[i]);
    // driver_read(inode, super->inodes + dirents[i].inode);
    // PrintInode(inode[0]);
  }
}

bool CheckNULL(char c)
{
  for(uint8_t i= 0;i<31; i++)
  {
    if(c == i)
      return true;
  }
  return false;
}
