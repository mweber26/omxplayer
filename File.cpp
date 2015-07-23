/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "linux/PlatformDefs.h"
#include <iostream>
#include <stdio.h>
#include "utils/StdString.h"

#include "File.h"

using namespace XFILE;
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef __GNUC__
#pragma warning (disable:4244)
#endif

//*********************************************************************************************
CFile::CFile()
{
  m_pFile = NULL;
  m_flags = 0;
  m_bPipe = false;
}

//*********************************************************************************************
CFile::~CFile()
{
  if(m_pFile && !m_bPipe)
    fclose(m_pFile);
}

//*********************************************************************************************
bool CFile::Open(const CStdString& strFileName, unsigned int flags)
{
  m_flags = flags;
  m_fileName = strFileName;

  if (strFileName.compare(0, 5, "pipe:") == 0)
  {
    m_bPipe = true;
    m_pFile = stdin;
    return true;
  }
  m_pFile = fopen64(strFileName.c_str(), "r");
  if(!m_pFile)
    return false;

  return true;
}

bool CFile::OpenForWrite(const CStdString& strFileName, bool bOverWrite)
{
  return false;
}

bool CFile::Exists(const CStdString& strFileName, bool bUseCache /* = true */)
{
  FILE *fp;

  if (strFileName.compare(0, 5, "pipe:") == 0)
    return true;

  fp = fopen64(strFileName.c_str(), "r");

  if(!fp)
    return false;

  fclose(fp);

  return true;
}

unsigned int CFile::Read(void *lpBuf, int64_t uiBufSize)
{
  unsigned int i;
  unsigned int ret = 0;
  int64_t offset = 0;

  if(!m_pFile)
    return 0;

  //get where the fread should always start
  offset = ftello64(m_pFile);

  for(i = 0; i < 100; i++)
  {
    ret = fread(lpBuf, 1, uiBufSize, m_pFile);
    //if we don't get what they want, try to reopen the file and try again
    if(ret < uiBufSize)
    {
      usleep(100000); //100ms
      fclose(m_pFile);
      m_pFile = fopen64(m_fileName.c_str(), "r");
      fseeko64(m_pFile, offset, SEEK_SET);
    }
	 else
	 	break;
  }

  return ret;
}

//*********************************************************************************************
void CFile::Close()
{
  if(m_pFile && !m_bPipe)
    fclose(m_pFile);
  m_pFile = NULL;
}

//*********************************************************************************************
int64_t CFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_pFile)
    return -1;

  return fseeko64(m_pFile, iFilePosition, iWhence);;
}

//*********************************************************************************************
int64_t CFile::GetLength()
{
  int64_t ret;

  //get the current offset
  int64_t offset = ftell(m_pFile);

  //close and reopen the file
  fclose(m_pFile);
  m_pFile = fopen64(m_fileName.c_str(), "r");

  //go to end and get the length
  fseeko64(m_pFile, 0, SEEK_END);
  ret = ftello64(m_pFile);

  //move back to the original offset
  fseeko64(m_pFile, offset, SEEK_SET);

  return ret;
}

//*********************************************************************************************
int64_t CFile::GetPosition()
{
  if (!m_pFile)
    return -1;

  return ftello64(m_pFile);
}

//*********************************************************************************************
int CFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  return -1;
}

int CFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE && m_pFile)
  {
    if (m_bPipe)
      return false;

    struct stat st;
    if (fstat(fileno(m_pFile), &st) == 0)
    {
      return !S_ISFIFO(st.st_mode);
    }
  }

  return -1;
}

bool CFile::IsEOF()
{
  if (!m_pFile)
    return -1;

  if (m_bPipe)
    return false;

  return feof(m_pFile) != 0;
}
