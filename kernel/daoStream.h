/*
// Dao Virtual Machine
// http://www.daovm.net
//
// Copyright (c) 2006-2013, Limin Fu
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DAO_STREAM_H
#define DAO_STREAM_H

#include<stdio.h>
#include<wchar.h>

#include"daoType.h"

#define IO_BUF_SIZE  512

enum DaoStreamModes
{
	DAO_IO_FILE = 1 ,
	DAO_IO_PIPE = 2 ,
	DAO_IO_STRING = 4 ,
	DAO_IO_READ = 1 ,
	DAO_IO_WRITE = 2
};


struct DaoStream
{
	DAO_CSTRUCT_COMMON;

	char        attribs;
	char        mode;
	char       *format;
	FILE       *file;
	DString    *streamString;
	DString    *fname;

	DaoUserStream *redirect;
};
DAO_DLL DaoType *dao_type_stream;

DAO_DLL DaoStream* DaoStream_New();
DAO_DLL void DaoStream_Delete( DaoStream *self );
DAO_DLL void DaoStream_Close( DaoStream *self );
DAO_DLL void DaoStream_Flush( DaoStream *self );

DAO_DLL void DaoStream_SetColor( DaoStream *self, const char *fgcolor, const char *bgcolor );

DAO_DLL void DaoStream_WriteChar( DaoStream *self, char val );
DAO_DLL void DaoStream_WriteInt( DaoStream *self, daoint val );
DAO_DLL void DaoStream_WriteFloat( DaoStream *self, double val );
DAO_DLL void DaoStream_WriteString( DaoStream *self, DString *val );
DAO_DLL void DaoStream_WriteMBS( DaoStream *self, const char *val );
DAO_DLL void DaoStream_WriteWCS( DaoStream *self, const wchar_t *val );
DAO_DLL void DaoStream_WritePointer( DaoStream *self, void *val );
DAO_DLL void DaoStream_WriteFormatedInt( DaoStream *self, daoint val, const char *format );
DAO_DLL void DaoStream_WriteNewLine( DaoStream *self );

DAO_DLL void DaoFile_WriteString( FILE *fout, DString *str );
DAO_DLL int DaoStream_ReadLine( DaoStream *self, DString *buf );
DAO_DLL int DaoFile_ReadLine( FILE *fin, DString *line );
DAO_DLL int DaoFile_ReadAll( FILE *fin, DString *all, int close );


#endif
