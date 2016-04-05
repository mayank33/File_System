*******File System is meant for Linux Machines Only**********



Instructions(For Debian and Ubuntu Machines) :

1. Install FUSE Libraries

% sudo apt-get install libfuse2 libfuse-dev

2. Mounting the file System

% ./MemoryFS_r -f <DirectoryPathToBeMounted>

3.Unmounting the file System

% fusermount -u -z <DirectoryPathToBeUnmounted>


