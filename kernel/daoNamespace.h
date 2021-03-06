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

#ifndef DAO_NAMESPACE_H
#define DAO_NAMESPACE_H

#include"daoBase.h"
#include"daoConst.h"
#include"daoString.h"
#include"daoArray.h"
#include"daoMap.h"
#include"daoType.h"


struct DaoNamespace
{
	DAO_DATA_COMMON;

	DaoVmSpace *vmSpace;

	DMap    *lookupTable; /* <DString*,size_t> */
	DArray  *namespaces; /* <DaoNamespace*> */
	DArray  *constants; /* <DaoConstant*>, global constants; */
	DArray  *variables; /* <DaoVariable*>, global variables; */
	DArray  *auxData;   /* mainly for GC */

	DaoRoutine *mainRoutine;
	DaoList    *argParams;

	DArray *mainRoutines; /* stdlib.eval() */
	DArray *definedRoutines; /* for DaoStudio IDE */

	void  *libHandle;
	int cstUser;
	int options;

	DMap   *localMacros; /* <DString*,DaoMacro*> */
	DMap   *globalMacros; /* <DString*,DaoMacro*> */
	DMap   *abstypes;     /* <DString*,DaoType*> */
	DMap   *moduleLoaders; /* <DString*,DaoModuleLoader> */
	DMap   *codeInliners; /* <DString*,DaoCodeInliner> */

	DString *file;
	DString *path;
	DString *name; /* path + file */
	DString *lang;
	DString *inputs; /* interactive inputs (load statements and some definitions) */
	DArray  *loadings;
	DArray  *sources;
	size_t   time;

	DaoProcess  *constEvalProcess;
	DaoRoutine  *constEvalRoutine;
};

DAO_DLL DaoNamespace* DaoNamespace_New( DaoVmSpace *vms, const char *name );
DAO_DLL void DaoNamespace_Delete( DaoNamespace *self );

DAO_DLL void DaoNamespace_SetName( DaoNamespace *self, const char *name );

DAO_DLL int DaoNamespace_FindConst( DaoNamespace *self, DString *name );
DAO_DLL int DaoNamespace_AddConst( DaoNamespace *self, DString *name, DaoValue *value, int pm );
DAO_DLL void DaoNamespace_SetConst( DaoNamespace *self, int index, DaoValue *value );
DAO_DLL DaoValue* DaoNamespace_GetConst( DaoNamespace *self, int i );

DAO_DLL int DaoNamespace_FindVariable( DaoNamespace *self, DString *name );
DAO_DLL int DaoNamespace_AddVariable( DaoNamespace *self, DString *name, DaoValue *var, DaoType *tp, int pm );
DAO_DLL int DaoNamespace_SetVariable( DaoNamespace *self, int index, DaoValue *var );
DAO_DLL DaoValue* DaoNamespace_GetVariable( DaoNamespace *self, int i );
DAO_DLL DaoType* DaoNamespace_GetVariableType( DaoNamespace *self, int i );

DAO_DLL void DaoNamespace_SetData( DaoNamespace *self, DString *name, DaoValue *value );
DAO_DLL DaoValue* DaoNamespace_GetData( DaoNamespace *self, DString *name );
DAO_DLL DaoValue* DaoNamespace_GetValue( DaoNamespace *self, daoint index );

DAO_DLL DaoClass* DaoNamespace_FindClass( DaoNamespace *self, DString *name );
DAO_DLL DaoNamespace* DaoNamespace_FindNamespace( DaoNamespace *self, DString *name );

DAO_DLL void DaoNamespace_UpdateLookupTable( DaoNamespace *self );
DAO_DLL int DaoNamespace_AddParent( DaoNamespace *self, DaoNamespace *parent );

DAO_DLL void DaoNamespace_AddConstNumbers( DaoNamespace *self, DaoNumItem *items );

DAO_DLL void DaoNamespace_ImportMacro( DaoNamespace *self, DString *lang );
DAO_DLL void DaoNamespace_AddMacro( DaoNamespace *self, DString *lang, DString *name, DaoMacro *macro );
DAO_DLL DaoMacro* DaoNamespace_FindMacro( DaoNamespace *self, DString *lang, DString *name );

DAO_DLL DaoModuleLoader DaoNamespace_FindModuleLoader( DaoNamespace *self, DString *name );
DAO_DLL DaoCodeInliner DaoNamespace_FindCodeInliner( DaoNamespace *self, DString *name );

DAO_DLL DaoType* DaoNamespace_FindType( DaoNamespace *self, DString *name );
DAO_DLL DaoType* DaoNamespace_FindTypeMBS( DaoNamespace *self, const char *name );
DAO_DLL DaoType* DaoNamespace_AddType( DaoNamespace *self, DString *name, DaoType *tp );
DAO_DLL void DaoNamespace_AddTypeConstant( DaoNamespace *self, DString *name, DaoType *tp );

DAO_DLL DaoType* DaoNamespace_GetType( DaoNamespace *self, DaoValue *p );
DAO_DLL DaoType* DaoNamespace_MakeType( DaoNamespace *self, const char *name,
		uint_t basic, DaoValue *pb, DaoType *nest[], int N );
DAO_DLL DaoType* DaoNamespace_MakeRoutType( DaoNamespace *self, DaoType *routype,
		DaoValue *vals[], DaoType *types[], DaoType *retp );

DAO_DLL DaoType* DaoNamespace_MakeValueType( DaoNamespace *self, DaoValue *value );
DAO_DLL DaoType* DaoNamespace_MakePairType( DaoNamespace *self, DaoType *first, DaoType *second );
DAO_DLL DaoType* DaoNamespace_MakePairValueType( DaoNamespace *self, DaoValue *first, DaoValue *second );
DAO_DLL DaoTuple* DaoNamespace_MakePair( DaoNamespace *self, DaoValue *first, DaoValue *second );

DAO_DLL DaoType* DaoNamespace_MakeEnumType( DaoNamespace *self, const char *symbols );
DAO_DLL DaoType* DaoNamespace_SymbolTypeAdd( DaoNamespace *self, DaoType *t1, DaoType *t2, int *value );
DAO_DLL DaoType* DaoNamespace_SymbolTypeSub( DaoNamespace *self, DaoType *t1, DaoType *t2, int *value );

DAO_DLL int DaoNamespace_SetupValues( DaoNamespace *self, DaoTypeBase *typer );
DAO_DLL void DaoMethods_Insert( DMap *methods, DaoRoutine *rout, DaoNamespace *ns, DaoType *host );
DAO_DLL int DaoNamespace_SetupMethods( DaoNamespace *self, DaoTypeBase *typer );
DAO_DLL DaoType* DaoNamespace_SetupType( DaoNamespace *self, DaoTypeBase *typer );

DAO_DLL DaoRoutine* DaoNamespace_ParseSignature( DaoNamespace *self, const char *proto, DaoParser *parser );
DAO_DLL DaoRoutine* DaoNamespace_MakeFunction( DaoNamespace *self, const char *proto, DaoParser *parser );

DAO_DLL DaoValue* DaoValue_FindAuxMethod( DaoValue *self, DString *name, DaoNamespace *nspace );
DAO_DLL DaoValue* DaoType_FindAuxMethod( DaoType *self, DString *name, DaoNamespace *nspace );

DAO_DLL void DaoNamespace_InitConstEvalData( DaoNamespace *self );

#endif
