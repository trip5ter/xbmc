#pragma once

#include "MythPointer.h"
#include "libcmyth.h"
#include <boost/shared_ptr.hpp>

template < class T > class MythPointer;

class MythFile 
{
public:
  MythFile();
  MythFile(cmyth_file_t myth_file);
  bool IsNull();
  int Read(void* buffer,long long length);
  long long Seek(long long offset, int whence);
  long long Duration();

private:
  boost::shared_ptr< MythPointer< cmyth_file_t > > m_file_t; 
};