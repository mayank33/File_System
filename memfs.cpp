#include<iostream>
#include<map>

using namespace std;
extern "C" {

#define FUSE_USE_VERSION  26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
	
/*-------------------
  Basic Functions, you need to implement
*/
typedef map<int, char> Contents;
typedef map<string, Contents> File;
typedef map<string,int > Directory;
typedef map<string,string>FDMap;
typedef map<string,string>DDMap;
static File files;
static Directory dirs;
static FDMap fdmaps;
static DDMap ddmaps;
long dirCount;
string currDir;

static string get_file_name(string path)
{
	unsigned found = path.find_last_of("/");
	return path.substr(found+1);	
}

Contents to_map(string data) {
	Contents data_map;
	int i = 0;
	
	for (string::iterator it = data.begin(); it < data.end(); ++it)
		data_map[i++] = *it;
		
	return data_map;
}
static bool find_file(string filename)
{
	bool found = files.find(filename) != files.end();
	return found;
}

static bool find_directory(string dirname)
{
	bool found =dirs.find(dirname) != dirs.end();
	for (Directory::iterator it = dirs.begin(); it != dirs.end(); it++)
			if(!strcmp(it->first.c_str(),dirname.c_str()))
				{
				}
	return found;
}

static int memfs_getattr(const char* path, struct stat* stbuf) 
{
	string filename = path;
	filename = get_file_name(filename);
	bool is_file=false,found=false;
	if(find_file(filename))
	{
		found=true;
		is_file=true;
	}
	else
	{
		if(find_directory(filename))
		{
			found=true;
		}
	}
	int value = 0;
	memset(stbuf, 0, sizeof(struct stat));
	time_t timer;
	stbuf->st_uid = getuid();
	time(&timer);
	stbuf->st_gid = getgid();	
	stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL) ;
	if((string)path == "/" || (found==true && is_file==false)) // For Directory
	 { 
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		
	} else if(found==true && is_file==true) { //An existing file is read
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = files[filename].size();
		
	} else {
		value = -ENOENT;
	}

	return value;
	 
}

static int memfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info* fi)
{
	//Implememting Single Directory So Only Root is Traversable
	if(!strcmp(path,"/"))
	{
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		for (DDMap::iterator it = ddmaps.begin(); it != ddmaps.end(); it++)
		{
			if(ddmaps[it->first]==currDir)
			filler(buf, it->first.c_str(), NULL, 0);
		}
		for (FDMap::iterator it = fdmaps.begin(); it != fdmaps.end(); it++) 
		{
			if(fdmaps[it->first]==currDir)			
			filler(buf, it->first.c_str(), NULL, 0); // Converting a C++ "string" to C pointer String 			
		}
		return 0;
		
	}
	else if(find_directory((get_file_name(path))))
	{		
	//Adding Current Directry to Traverse
	for (FDMap::iterator it = fdmaps.begin(); it != fdmaps.end(); it++) 
		{
			if(fdmaps[it->first]==currDir)
				filler(buf, it->first.c_str(), NULL, 0); // Converting a C++ "string" to C pointer String 
		}
	for (DDMap::iterator it = ddmaps.begin(); it != ddmaps.end(); it++)
		{
			if(ddmaps[it->first]==currDir)
			filler(buf, it->first.c_str(), NULL, 0);
		}return 0; }
	else
	return -ENOENT;
}


static int memfs_open(const char* path, struct fuse_file_info* fi)
{
	/*No Need To Implement Open*/
	string filename=get_file_name(path);
	if(!find_file(filename))
	{
		return -ENOENT; // No such File Exist !!
	}
	return 0;
}

static int memfs_read(const char* path, char* buf, size_t size, off_t offset,struct fuse_file_info* fi)
{
	string filename=get_file_name(path);
	int i;
	if (find_file(get_file_name(path))==false)
		return -ENOENT;
		
	Contents &file = files[filename];
	size_t len = file.size();
	if (offset >= len)
		return -EINVAL;
	else if(offset>=0)
	{
		size=len-offset;
			
	}
	for(i=0;i<(int)size;i++)
		{
			buf[i] = file[(int)offset + i];
		}
		
	return (int)size;
	
}
	
int memfs_write(const char* path, const char* data, size_t size, off_t offset, struct fuse_file_info*) 
{
	string filename = get_file_name(path);
	
	
	if( !find_file(filename) ) {
		return -ENOENT;
	}

	Contents &file = files[filename];
	
	for(size_t i = 0; i < size; ++i)
		file[offset + i] = data[i];
		
	return size;
}

int memfs_unlink(const char *pathname) 
{
	string filename = get_file_name(pathname);
	int value=0;
	//To check if the File we are creating exists
	if( find_file(filename)==false )
	{
		value=-EEXIST;
		return value;
	}
	
	string backup_file=filename.append("~");
	files.erase(get_file_name(pathname));
	fdmaps.erase(get_file_name(pathname));
	files.erase(backup_file);
	return value;
}

int memfs_create(const char* path, mode_t mode, struct fuse_file_info *)
{
	string filename = get_file_name(path);
	
	//To check if the File we are creating exists
	if( find_file(filename)==true ) {
		return -EEXIST;
	}
	
	if( (mode & S_IFREG) == 0)	{		
		return -EINVAL;
	}
	Contents data_map;
	data_map[0]='\0';
	files[filename] = data_map;
	fdmaps[filename]=currDir;
	return 0;
}

/*-------------
 Implemented functions
 -------------*/   
int memfs_fgetattr(const char* path, struct stat* stbuf, struct fuse_file_info *)
{
	return memfs_getattr(path, stbuf);
}

int memfs_opendir(const char* path, struct fuse_file_info *)
{  
	if(find_directory(get_file_name(path)))
		currDir=get_file_name(path);
	if (!strcmp(path,"/"))
		currDir="root";
	return 0;
}

int memfs_access(const char* path, int) {  
	cout << "memfs_access("<<path<<") access granted" << endl; 
	return 0; 
}
/*---------------------
  These are other functions that you may have to implement
 ----------------------*/
int memfs_truncate(const char* path, off_t length)
{
	string filename=path;
	filename=get_file_name(path);
	size_t size = files[filename].size();
	if ((int)size < (int)length)
	{
		for(size_t i = size; i < length; ++i)
			files[filename][i] = '\0';
		return 0;		
	}
	else
	{
			
	Contents file1 = files[filename];
	Contents file2;
	
	for(size_t i =0; i < length; ++i)
		file2[i] = file1[i];
	files[filename]=file2;
		return 0;		
	}
}

int memfs_mknod(const char* path, mode_t mode, dev_t dev) {  cout << "memfs_mknod not implemented" << endl; return -EINVAL; }
int memfs_mkdir(const char *path, mode_t mode)
{
	string dir_name=get_file_name(path);
	if(find_directory(path))
		return -EEXIST;
	int last_node;
	//for (Directory::iterator it = dirs.begin(); it != dirs.end(); it++)
	//last_node=it->second;	
	dirs[dir_name]=dirCount++;
	mode=0777;
	ddmaps[dir_name]=currDir;
	return 0;
}
int memfs_rmdir(const char * pathname)
{
	string dirname = get_file_name(pathname);
	int value=0;
	//To check if the File we are creating exists
	if( !find_directory(dirname) )
	{
		value=-EEXIST;
	}
	for (FDMap::iterator it = fdmaps.begin(); it != fdmaps.end(); it++) 
	{		
		if(it->second==dirname)
		{
			files.erase(it->first);
			fdmaps.erase(it->first);			
		}
	}
	ddmaps.erase(dirname);
	dirs.erase(dirname);	
	return value;
}
int memfs_symlink(const char *oldpath, const char * newpath)
{
	string old=oldpath,new1=newpath;
	old=get_file_name(old);	
	if(!find_file(old))
		return -EEXIST;
	new1=get_file_name(newpath);
	Contents data_map;	
	files[new1] = files[old];
	fdmaps[new1]=currDir;
	return 0;
	
}
int memfs_rename(const char * old_path, const char * new_path) 
{  
	string old=old_path,file,dir,replace=new_path;
	old=get_file_name(old);
	replace=get_file_name(replace);
	bool is_file=false,is_dir=false;
	for (FDMap::iterator it = fdmaps.begin(); it != fdmaps.end(); it++) 
		{
			if(fdmaps[it->first]==currDir)
				{
					is_file=true; 
					file=it->first;
					break;
				}
				
		}
	for (DDMap::iterator it = ddmaps.begin(); it != ddmaps.end() && is_file==false; it++)
		{
			if(ddmaps[it->first]==currDir)
			{
				is_dir=true;
				dir=it->first;
				break;
			}
		}
	if(!is_file && !is_dir)
		return -EEXIST;
		
	if(is_file)
	{
		fdmaps.erase(file);
		fdmaps[replace]=currDir;
		files[replace]=files[old];
		files.erase(old);
		return 0;
	}
	
}
int memfs_link(const char *, const char *) {  cout << "memfs_link not implemented" << endl; return -EINVAL; }
int memfs_chmod(const char *, mode_t) {  cout << "memfs_chmod not implemented" << endl; return -EINVAL; }
int memfs_chown(const char *, uid_t, gid_t) {  cout << "memfs_chown not implemented" << endl; return -EINVAL; }
int memfs_utime(const char * path, struct utimbuf * ut)
{
	//struct stat stbuf;
	//if (memfs_getattr(path,&stbuf) != 0)
		//return -EINVAL;
	//stbuf.st_atime=ut->actime;   
    //stbuf.st_mtime=ut->modtime; 
    if(!find_directory(get_file_name((string) path)) && !find_file(get_file_name((string) path)))
    {
		return -EEXIST;
	}
	time_t timer;
	localtime(&timer);
	ut->actime=timer;
	ut->modtime=timer;
	return 0;
	
}
int memfs_utimens(const char *, const struct timespec tv[2])
{
	
}
int memfs_bmap(const char *, size_t blocksize, uint64_t *idx) {  cout << "memfs_bmap not implemented" << endl; return -EINVAL; }
int memfs_setxattr(const char *, const char *, const char *, size_t, int) {  cout << "memfs_setxattr not implemented" << endl; return -EINVAL; }


//Fuse basically maps your defined functions to filesystem functions, and here is how 
static struct fuse_operations memfs_oper;

int main(int argc, char** argv) {
	
	dirCount=0;
	currDir="root";
	
	//Mapping your ops to Fuse ops
	memfs_oper.getattr	= memfs_getattr;
	memfs_oper.readdir 	= memfs_readdir;
	memfs_oper.open   	= memfs_open;
	memfs_oper.read   	= memfs_read;
	memfs_oper.mknod	= memfs_mknod;
	memfs_oper.write 	= memfs_write;
	memfs_oper.unlink 	= memfs_unlink;	
	memfs_oper.setxattr = memfs_setxattr;
	memfs_oper.mkdir = memfs_mkdir;
	memfs_oper.rmdir = memfs_rmdir;
	memfs_oper.symlink = memfs_symlink;
	memfs_oper.rename = memfs_rename;
	memfs_oper.link = memfs_link;
	memfs_oper.chmod = memfs_chmod;
	memfs_oper.chown = memfs_chown;
	memfs_oper.truncate = memfs_truncate;
	memfs_oper.utime = memfs_utime;
	memfs_oper.opendir = memfs_opendir;
	memfs_oper.access = memfs_access;
	memfs_oper.create = memfs_create;
	memfs_oper.fgetattr = memfs_fgetattr;
	memfs_oper.utimens = memfs_utimens;
	memfs_oper.bmap = memfs_bmap;
	
	
	//start the filesystem
	return fuse_main(argc, argv, &memfs_oper, NULL);
}

}
