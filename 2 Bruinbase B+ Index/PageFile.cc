/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include "Bruinbase.h"
#include "PageFile.h"
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using std::string;

// initialize static private member of PageFile Class
int PageFile::readCount = 0;
int PageFile::writeCount = 0;
int PageFile::cacheClock = 1;
struct PageFile::cacheStruct PageFile::readCache[PageFile::CACHE_COUNT];

// default constructor, set file id to -1, page id to 0
PageFile::PageFile() 
{ 
  fd = -1; 
  epid = 0; 
}

// constructor with parameter, given a file and its mode
PageFile::PageFile(const string& filename, char mode)
{
  fd = -1;
  epid = 0;
// call in-class function open. .c_str() convert to C-string
  open(filename.c_str(), mode);
}

// RC, return type (defined in bruinbase.h, return code)
RC PageFile::open(const string& filename, char mode)
{
  RC   rc;
// oflag: unix file flag
  int  oflag;
    
/** http://pubs.opengroup.org/onlinepubs/009695399/functions/stat.html
 http://linux.die.net/man/2/fstat
 stat - get file status
 int stat(const char *restrict path, struct stat *restrict buf);
 The stat() function shall obtain information about the named file and write it to the area pointed to by the buf
 argument. The path argument points to a pathname naming a file.
 */
  struct stat statbuf;

  if (fd > 0) return RC_FILE_OPEN_FAILED;

  // set the unix file flag depending on the file mode
  switch (mode) {
  case 'r':
  case 'R':
    oflag = O_RDONLY;
    break;
  case 'w':
  case 'W':
    oflag = (O_RDWR|O_CREAT);
    break;
  default:
    return RC_INVALID_FILE_MODE;
  }

  // open the file
  fd = ::open(filename.c_str(), oflag, 0644);
  if (fd < 0) { fd = -1; return RC_FILE_OPEN_FAILED; }

  // get the size of the file to set the end pid
  rc = ::fstat(fd, &statbuf);
  if (rc < 0) { ::close(fd); fd = -1; return RC_FILE_OPEN_FAILED; }
// st_size = total bytes, epid is ending page number
  epid = statbuf.st_size / PAGE_SIZE;

  return 0;
}

RC PageFile::close()
{
  if (fd <= 0) return RC_FILE_CLOSE_FAILED;

  // close the file
  if (::close(fd) < 0) return RC_FILE_CLOSE_FAILED;

  // evict all cached pages for this file
  for (int i = 0; i < CACHE_COUNT; i++) {
    if (readCache[i].fd == fd && readCache[i].lastAccessed != 0) {
       readCache[i].fd = 0;
       readCache[i].pid = 0;
       readCache[i].lastAccessed = 0;
    }
  }

  // set the fd and epid to the initial state
  fd = -1; 
  epid = 0;
  return 0;
}

PageId PageFile::endPid() const 
{
  return epid;
}

RC PageFile::seek(PageId pid) const
{
  return (::lseek(fd, pid * PAGE_SIZE, SEEK_SET) < 0) ? RC_FILE_SEEK_FAILED : 0;
}

RC PageFile::write(PageId pid, const void* buffer)
{
  RC rc;
  if (pid < 0) return RC_INVALID_PID; 

  // seek to the location of the page
  if ((rc = seek(pid)) < 0) return rc;

  // write the buffer to the disk page
  if (::write(fd, buffer, PAGE_SIZE) < 0) return RC_FILE_WRITE_FAILED;

  // if the page is in read cache, invalidate it
  for (int i = 0; i < CACHE_COUNT; i++) {
    if (readCache[i].fd == fd && readCache[i].pid == pid &&
        readCache[i].lastAccessed != 0) {
       readCache[i].fd = 0;
       readCache[i].pid = 0;
       readCache[i].lastAccessed = 0;
       break;
    }
  }

  // if the written pid >= end pid, update the end pid
  if (pid >= epid) epid = pid + 1;

  // increase page write count
  writeCount++;

  return 0;
}

RC PageFile::read(PageId pid, void* buffer) const
{
  RC rc;

  if (pid < 0 || pid >= epid) return RC_INVALID_PID; 

  //
  // if the page is in cache, read it from there
  //
  for (int i = 0; i < CACHE_COUNT; i++) {
    if (readCache[i].fd == fd && readCache[i].pid == pid && 
        readCache[i].lastAccessed != 0) {
       memcpy(buffer, readCache[i].buffer, PAGE_SIZE);
       readCache[i].lastAccessed = ++cacheClock;
       return 0;
    }
  }

  // seek to the page
  if ((rc = seek(pid)) < 0) return rc;
  
  // find the cache slot to evict
  int toEvict = 0; 
  for (int i = 0; i < CACHE_COUNT; i++) {
    if (readCache[i].lastAccessed == 0) {
      toEvict = i;
      break;
    }
    if (readCache[i].lastAccessed < readCache[toEvict].lastAccessed) {
      toEvict = i;
    }
  }
  readCache[toEvict].fd = fd;
  readCache[toEvict].pid = pid;
  readCache[toEvict].lastAccessed = ++cacheClock;
 
  // read the page to cache first and copy it to the buffer
  if (::read(fd, readCache[toEvict].buffer, PAGE_SIZE) < 0) {
    return RC_FILE_READ_FAILED;
  }
  memcpy(buffer, readCache[toEvict].buffer, PAGE_SIZE);

  // increase the page read count
  readCount++;

  return 0;
}
