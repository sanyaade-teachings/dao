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


#include<stdio.h>
#include<string.h>
#include<time.h>
#include<math.h>
#include<assert.h>
#include"daoType.h"
#include"daoValue.h"
#include"daoThread.h"
#include"daoRoutine.h"
#include"daoObject.h"
#include"daoVmspace.h"
#include"daoProcess.h"
#include"daoGC.h"
#include"daoTasklet.h"

#define MIN_TIME  1E-27

#ifdef DAO_WITH_CONCURRENT


DaoTaskEvent* DaoTaskEvent_New()
{
	DaoTaskEvent *self = (DaoTaskEvent*) dao_calloc( 1, sizeof(DaoTaskEvent) );
	return self;
}
void DaoTaskEvent_Reset( DaoTaskEvent *self )
{
	self->type = 0;
	self->state = 0;
	self->timeout = 0;
	self->auxiliary = 0;
	self->expiring = -1.0;
	GC_DecRC( self->future );
	GC_DecRC( self->channel );
	GC_DecRC( self->selected );
	GC_DecRC( self->message );
	GC_DecRC( self->selects );
	self->future = NULL;
	self->channel = NULL;
	self->selected = NULL;
	self->message = NULL;
	self->selects = NULL;
}
void DaoTaskEvent_Delete( DaoTaskEvent *self )
{
	DaoTaskEvent_Reset( self );
	dao_free( self );
}
void DaoTaskEvent_Init( DaoTaskEvent *self, int T, int S, DaoFuture *F, DaoChannel *C )
{
	GC_ShiftRC( F, self->future );
	GC_ShiftRC( C, self->channel );
	self->type = T;
	self->state = S;
	self->future = F;
	self->channel = C;
}



DaoChannel* DaoChannel_New( DaoType *type, int dtype )
{
	DaoChannel *self = (DaoChannel*) dao_calloc( 1, sizeof(DaoChannel) );
	if( dtype ) type = DaoCdataType_Specialize( dao_type_channel, & type, type != NULL );
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->buffer = DArray_New(D_VALUE);
	return self;
}




DaoFuture* DaoFuture_New( DaoType *type, int vatype )
{
	DaoFuture *self = (DaoFuture*) dao_calloc( 1, sizeof(DaoFuture) );
	if( vatype ) type = DaoCdataType_Specialize( dao_type_future, & type, type != NULL );
	DaoCstruct_Init( (DaoCstruct*) self, type );
	GC_IncRC( dao_none_value );
	self->state = DAO_CALL_PAUSED;
	self->value = dao_none_value;
	return self;
}


static int DaoValue_CheckCtype( DaoValue *self, DaoType *type )
{
	if( self->type != DAO_CSTRUCT && self->type != DAO_CDATA ) return 0;
	if( self->type != type->tid ) return 0;
	return self->xCstruct.ctype->typer == type->typer;
}




typedef struct DaoCallThread   DaoCallThread;
typedef struct DaoCallServer   DaoCallServer;


struct DaoCallThread
{
	DThread       thread;
	DThreadData  *thdData;

	DThreadTask   taskFunc;  /* first task; */
	void         *taskParam;
};

static DaoCallThread* DaoCallThread_New( DThreadTask func, void *param );
static void DaoCallThread_Run( DaoCallThread *self );

struct DaoCallServer
{
	DThread  timer;
	DMutex   mutex;
	DCondVar condv;
	DCondVar condv2;

	int finishing;
	int timing;
	int total;
	int idle;
	int stopped;

	DArray  *threads;

	DArray  *functions;  /* list of DThreadTask function pointers */
	DArray  *parameters; /* list of void* */
	DArray  *events;     /* list of DaoTaskEvent* */
	DArray  *events2;    /* list of DaoTaskEvent* */
	DMap    *waitings;   /* timed waiting: <DaoComplex,DaoTaskEvent*> */
	DMap    *active;     /* map of DaoObject* or DaoProcess* keys */
	DMap    *pending;    /* map of pointers from ::parameters, ::events and ::events2 */

	DArray  *caches;

	DaoComplex   timestamp;  /* (time,index); */
	DaoVmSpace  *vmspace;
};
static DaoCallServer *daoCallServer = NULL;

static DaoCallThread* DaoCallThread_New( DThreadTask func, void *param )
{
	DaoCallThread *self = (DaoCallThread*)dao_malloc( sizeof(DaoCallThread) );
	self->thdData = NULL;
	self->taskFunc = func;
	self->taskParam = param;
	DThread_Init( & self->thread );
	return self;
}
static void DaoCallThread_Delete( DaoCallThread *self )
{
	// XXX self->thdData
	DThread_Destroy( & self->thread );
	dao_free( self );
}
static DaoCallServer* DaoCallServer_New( DaoVmSpace *vms )
{
	DaoComplex com = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
	DaoCallServer *self = (DaoCallServer*)dao_malloc( sizeof(DaoCallServer) );
	DMutex_Init( & self->mutex );
	DCondVar_Init( & self->condv );
	DCondVar_Init( & self->condv2 );
	DThread_Init( & self->timer );
	self->finishing = 0;
	self->timing = 0;
	self->total = 0;
	self->idle = 0;
	self->stopped = 0;
	self->threads = DArray_New(0);
	self->functions = DArray_New(0);
	self->parameters = DArray_New(0);
	self->events = DArray_New(0);
	self->events2 = DArray_New(0);
	self->waitings = DMap_New(D_VALUE,0);
	self->pending = DHash_New(0,0);
	self->active = DHash_New(0,0);
	self->caches = DArray_New(0);
	self->vmspace = vms;
	self->timestamp = com;
	return self;
}
static void DaoCallServer_Delete( DaoCallServer *self )
{
	daoint i;
	for(i=0; i<self->threads->size; i++){
		DaoCallThread_Delete( (DaoCallThread*)self->threads->items.pVoid[i] );
	}
	for(i=0; i<self->caches->size; ++i){
		DaoTaskEvent_Delete( (DaoTaskEvent*) self->caches->items.pVoid[i] );
	}
	DArray_Delete( self->threads );
	DArray_Delete( self->functions );
	DArray_Delete( self->parameters );
	DArray_Delete( self->events );
	DArray_Delete( self->events2 );
	DMap_Delete( self->waitings );
	DMap_Delete( self->pending );
	DMap_Delete( self->active );
	DMutex_Destroy( & self->mutex );
	DCondVar_Destroy( & self->condv );
	DCondVar_Destroy( & self->condv2 );
	DThread_Destroy( & self->timer );
	dao_free( self );
}

static void DaoCallServer_Timer( void *p );

static void DaoCallServer_Init( DaoVmSpace *vms )
{
	DaoCGC_Start();
	daoCallServer = DaoCallServer_New( vms );
	DThread_Start( & daoCallServer->timer, (DThreadTask) DaoCallServer_Timer, NULL );
}
static DaoCallServer* DaoCallServer_TryInit( DaoVmSpace *vms )
{
	if( daoCallServer == NULL ) DaoCallServer_Init( vms );
	return daoCallServer;
}

static DaoTaskEvent* DaoCallServer_MakeEvent()
{
	DaoTaskEvent *event;
	DaoCallServer *server = daoCallServer;
	DMutex_Lock( & server->mutex );
	event = (DaoTaskEvent*) DArray_PopBack( server->caches );
	if( event == NULL ) event = DaoTaskEvent_New();
	DMutex_Unlock( & server->mutex );
	return event;
}
/* Lock daoCallServer::mutex before calling this function: */
static void DaoCallServer_CacheEvent( DaoTaskEvent *event )
{
	DaoCallServer *server = daoCallServer;
	DaoTaskEvent_Reset( event );
	DArray_PushBack( server->caches, event );
}
static void DaoCallServer_AddThread( DThreadTask func, void *param )
{
	DaoCallThread *calth;
	DaoCallServer_TryInit( mainVmSpace );
	calth = DaoCallThread_New( func, param );
	DMutex_Lock( & daoCallServer->mutex );
	daoCallServer->total += 1;
	DArray_Append( daoCallServer->threads, calth );
	DMutex_Unlock( & daoCallServer->mutex );
	DThread_Start( & calth->thread, (DThreadTask) DaoCallThread_Run, calth );
}
static void DaoCallServer_TryAddThread( DThreadTask func, void *param, int todo )
{
	int max = daoConfig.cpu > 2 ? daoConfig.cpu : 2;
	int total = daoCallServer->total;
	if( total < 2 ) DaoCallServer_AddThread( NULL, NULL );
	if( todo == 0 ) return;
	if( total < max || todo > pow( total + max + 10, 5 ) ) DaoCallServer_AddThread( NULL, NULL );
}
static int DaoTaskEvent_CheckSelect( DaoTaskEvent *self )
{
	DNode *it;
	int closed = 0;
	int move = 0;
	for(it=DaoMap_First(self->selects); it; it=DaoMap_Next(self->selects,it)){
		if( DaoValue_CheckCtype( it->key.pValue, dao_type_channel ) ){
			DaoChannel *chan = (DaoChannel*) it->key.pValue;
			move = chan->buffer->size > 0;
			closed += chan->cap == 0;
		}else{
			DaoFuture *fut = (DaoFuture*) it->key.pValue;
			move = fut->state == DAO_CALL_FINISHED;
		}
		if( move ) break;
	}
	if( self->selects->items->size == closed ) move = 1;
	return move;
}
static void DaoCallServer_ActivateEvents()
{
	DaoCallServer *server = daoCallServer;
	char message[128];
	daoint i, j;

	if( server->idle != server->total ) return;
	if( server->events->size != 0 ) return;
	if( server->events2->size == 0 ) return;

#ifdef DEBUG
	sprintf( message, "WARNING: activating events (%i,%i,%i,%i)!\n", server->total,
			server->idle, (int)server->events->size, (int)server->events2->size );
	DaoStream_WriteMBS( mainVmSpace->errorStream, message );
#endif
	for(i=0; i<server->events2->size; ++i){
		DaoTaskEvent *event = (DaoTaskEvent*) server->events2->items.pVoid[i];
		DaoChannel *chan = event->channel;
		DaoFuture *fut = event->future;
		int move = 0, closed = 0;
		switch( event->type ){
		case DAO_EVENT_WAIT_TASKLET :
			move = fut->precond == NULL || fut->precond->state == DAO_CALL_FINISHED;
			break;
		case DAO_EVENT_WAIT_RECEIVING :
			move = chan->buffer->size > 0;
			if( chan->cap <= 0 && chan->buffer->size == 0 ) move = 1;
			break;
		case DAO_EVENT_WAIT_SENDING :
			move = chan->buffer->size < chan->cap;
			break;
		case DAO_EVENT_WAIT_SELECT :
			if( event->selects == NULL ) continue;
			move = DaoTaskEvent_CheckSelect( event );
			break;
		default: break;
		}
		if( move ){
			DArray_Append( server->events, event );
			DArray_Erase( server->events2, i, 1 );
			i -= 1;
		}
	}
	DCondVar_Signal( & server->condv );
}
static void DaoCallServer_Timer( void *p )
{
	DaoCallServer *server = daoCallServer;
	double time = 0.0;
	daoint i, timeout;

	server->timing = 1;
	while( server->finishing == 0 || server->stopped != server->total ){
		DMutex_Lock( & server->mutex );
		while( server->waitings->size == 0 ){
			if( server->idle == server->total && server->events2->size ){
				DaoCallServer_ActivateEvents();
			}
			if( server->finishing && server->stopped == server->total ) break;
			DCondVar_TimedWait( & server->condv2, & server->mutex, 0.01 );
		}
		if( server->waitings->size ){
			DNode *node = DMap_First( server->waitings );
			time = node->key.pValue->xComplex.value.real;
			time -= Dao_GetCurrentTime();
			/* wait the right amount of time for the closest arriving timeout: */
			if( time > 0 ) DCondVar_TimedWait( & server->condv2, & server->mutex, time );
		}
		DMutex_Unlock( & server->mutex );
		if( server->finishing && server->stopped == server->total ) break;

		DMutex_Lock( & server->mutex );
		if( server->waitings->size ){ /* a new wait timed out: */
			DNode *node = DMap_First( server->waitings );
			time = Dao_GetCurrentTime();
			if( node->key.pValue->xComplex.value.real < time ){
				DaoTaskEvent *event = (DaoTaskEvent*) node->value.pVoid;
				event->state = DAO_EVENT_RESUME;
				event->timeout = 1;
				event->expiring = MIN_TIME;
				DArray_Append( server->events, node->value.pVoid );
				DMap_EraseNode( server->waitings, node );
			}
		}
		DCondVar_Signal( & server->condv );
		DMutex_Unlock( & server->mutex );
	}
	server->timing = 0;
}

void DaoCallServer_AddTask( DThreadTask func, void *param, int now )
{
	int scheduled = 0;
	DaoCallServer *server = DaoCallServer_TryInit( mainVmSpace );
	DMutex_Lock( & server->mutex );
	if( server->idle > server->parameters->size || now == 0 ){
		scheduled = 1;
		DArray_Append( server->functions, func );
		DArray_Append( server->parameters, param );
		DMap_Insert( server->pending, param, NULL );
		DCondVar_Signal( & server->condv );
	}
	DMutex_Unlock( & server->mutex );
	if( scheduled ){
		if( now == 0 ) DaoCallServer_TryAddThread( NULL, NULL, server->parameters->size );
	}else{
		DaoCallServer_AddThread( func, param );
	}
}
static void DaoCallServer_AddEvent( DaoTaskEvent *event )
{
	DaoCallServer *server = daoCallServer;
	DArray_Append( server->events, event );
	DMap_Insert( server->pending, event, NULL );
}
static void DaoCallServer_Add( DaoTaskEvent *event )
{
	DaoCallServer *server = daoCallServer;
	DMutex_Lock( & server->mutex );
	DaoCallServer_AddEvent( event );
	DCondVar_Signal( & server->condv );
	DMutex_Unlock( & server->mutex );
	DaoCallServer_TryAddThread( NULL, NULL, server->pending->size );
}
void DaoCallServer_AddCall( DaoProcess *caller )
{
	DaoTaskEvent *event;
	DaoProcess *callee = DaoVmSpace_AcquireProcess( caller->vmSpace );
	DaoStackFrame *frame = caller->topFrame;
	DaoType *type = (DaoType*) frame->routine->routType->aux;
	DaoFuture *future = DaoFuture_New( type, 1 );
	DaoValue **params = caller->stackValues + caller->topFrame->stackBase;
	int i, count = caller->topFrame->parCount;

	future->state = DAO_CALL_PAUSED;
	future->actor = caller->topFrame->object;
	GC_IncRC( future->actor );

	GC_ShiftRC( future, callee->future );
	callee->future = future;
	future->process = callee;
	GC_IncRC( future->process );

	callee->parCount = count;
	for(i=0; i<count; ++i) DaoValue_Copy( params[i], & callee->paramValues[i] );
	if( caller->topFrame->routine->body ){
		DaoProcess_PushRoutine( callee, caller->topFrame->routine, future->actor );
	}else{
		DaoProcess_PushFunction( callee, caller->topFrame->routine );
	}

	DaoCallServer_TryInit( mainVmSpace );;
	event = DaoCallServer_MakeEvent();
	DaoTaskEvent_Init( event, DAO_EVENT_RESUME_TASKLET, DAO_EVENT_RESUME, future, NULL );

	DaoProcess_PopFrame( caller );
	DaoProcess_PutValue( caller, (DaoValue*) future );

	DaoCallServer_Add( event );
}
DaoFuture* DaoProcess_GetInitFuture( DaoProcess *self )
{
	if( self->future ) return self->future;

	self->future = DaoFuture_New( NULL, 1 );
	self->future->process = self;
	GC_IncRC( self->future );
	GC_IncRC( self );
	return self->future;
}
void DaoCallServer_MarkActiveProcess( DaoProcess *process, int active )
{
	DaoCallServer *server = daoCallServer;

	if( daoCallServer == NULL ) return;
	if( process->active == active ) return;

	DMutex_Lock( & server->mutex );
	if( active ){
		DMap_Insert( server->active, process, NULL );
		process->active = 1;
	}else{
		DMap_Erase( server->active, process );
		process->active = 0;
	}
	DMutex_Unlock( & server->mutex );
}
void DaoCallServer_AddTimedWait( DaoProcess *wait, DaoTaskEvent *event, double timeout )
{
	DaoCallServer *server = daoCallServer;

	/*
	// The "wait" process may not be running in the thread pool,
	// so it may have not been added to active process list.
	// It is necessary to add it to the active list now,
	// to avoid it being activated immediately after it is blocked.
	// Activating it immediately may cause a race condition,
	// because it may have not been blocked completely
	// (namely, it may be still running).
	*/
	DaoCallServer_MarkActiveProcess( wait, 1 );

	DMutex_Lock( & server->mutex );
	if( timeout >= 1E-27 ){
		server->timestamp.value.real = timeout + Dao_GetCurrentTime();
		server->timestamp.value.imag += 1;
		event->expiring = server->timestamp.value.real;
		DMap_Insert( server->waitings, & server->timestamp, event );
		DMap_Insert( server->pending, event, NULL );
		DCondVar_Signal( & server->condv2 );
	}else{
		event->expiring = -1.0;
		DaoCallServer_AddEvent( event );
		DCondVar_Signal( & server->condv );
	}
	DMutex_Unlock( & server->mutex );
}
void DaoCallServer_AddWait( DaoProcess *wait, DaoFuture *pre, double timeout )
{
	DaoTaskEvent *event;
	DaoCallServer *server = DaoCallServer_TryInit( mainVmSpace );;
	DaoFuture *future = DaoProcess_GetInitFuture( wait );

	GC_ShiftRC( pre, future->precond );
	future->precond = pre;
	future->state = DAO_CALL_PAUSED;

	event = DaoCallServer_MakeEvent();
	DaoTaskEvent_Init( event, DAO_EVENT_WAIT_TASKLET, DAO_EVENT_WAIT, future, NULL );

	DaoCallServer_AddTimedWait( wait, event, timeout );
}
static int DaoCallServer_CheckEvent( DaoTaskEvent *event, DaoFuture *fut, DaoChannel *chan )
{
	DaoTaskEvent event2 = *event;
	daoint i, move = 0, closed = 0;
	switch( event->type ){
	case DAO_EVENT_WAIT_TASKLET :
		move = event->future->precond == fut;
		break;
	case DAO_EVENT_WAIT_RECEIVING :
		if( event->channel == chan ){
			move = chan->buffer->size > 0;
			move |= chan->cap <= 0 && chan->buffer->size == 0;
		}
		break;
	case DAO_EVENT_WAIT_SENDING :
		move = event->channel == chan && chan->buffer->size < chan->cap;
		break;
	case DAO_EVENT_WAIT_SELECT :
		if( event->selects == NULL ) break;
		if( fut  ) move |= DMap_Find( event->selects->items, fut ) != NULL;
		if( chan ) move |= DMap_Find( event->selects->items, chan ) != NULL;
		//move = DaoTaskEvent_CheckSelect( event );
		break;
	default: break;
	}
	return move;
}
/*
// Only activate one event per channel:
*/
void DaoChannel_ActivateEvent( DaoChannel *self, int type )
{
	DaoCallServer *server = daoCallServer;
	DNode *node;
	daoint i;

	for(i=0; i<server->events2->size; ++i){
		DaoTaskEvent *event = (DaoTaskEvent*) server->events2->items.pVoid[i];
		if( event->type != type ) continue;
		if( DaoCallServer_CheckEvent( event, NULL, self ) ){
			DArray_Append( server->events, event );
			DArray_Erase( server->events2, i, 1 );
			return;
		}
	}
	for(node=DMap_First(server->waitings); node; node=DMap_Next(server->waitings,node)){
		DaoTaskEvent *event = (DaoTaskEvent*) node->value.pValue;
		if( event->type != type ) continue;
		if( DaoCallServer_CheckEvent( event, NULL, self ) ){
			DArray_Append( server->events, event );
			DMap_EraseNode( server->waitings, node );
			return;
		}
	}
}
/*
// Activate all events waiting on a future value:
*/
void DaoFuture_ActivateEvent( DaoFuture *self )
{
	DaoCallServer *server = daoCallServer;
	DArray *array = DArray_New(0);
	DNode *node;
	daoint i;

	DMutex_Lock( & server->mutex );
	for(i=0; i<server->events2->size; ++i){
		DaoTaskEvent *event = (DaoTaskEvent*) server->events2->items.pVoid[i];
		if( DaoCallServer_CheckEvent( event, self, NULL ) ){
			event->state = DAO_EVENT_RESUME;
			DArray_Append( server->events, event );
			DArray_Erase( server->events2, i, 1 );
			i -= 1;
		}
	}
	for(node=DMap_First(server->waitings); node; node=DMap_Next(server->waitings,node)){
		DaoTaskEvent *event = (DaoTaskEvent*) node->value.pValue;
		/* remove from timed waiting list: */
		if( DaoCallServer_CheckEvent( event, self, NULL ) ){
			event->state = DAO_EVENT_RESUME;
			DArray_Append( server->events, event );
			DArray_Append( array, node->key.pVoid );
		}
	}
	for(i=0; i<array->size; i++) DMap_Erase( server->waitings, array->items.pVoid[i] );
	DCondVar_Signal( & server->condv );
	DMutex_Unlock( & server->mutex );
	DArray_Delete( array );
}
static DaoFuture* DaoCallServer_GetNextFuture()
{
	DaoCallServer *server = daoCallServer;
	DaoFuture *first, *future, *precond;
	DArray *events = server->events;
	DMap *pending = server->pending;
	DMap *active = server->active;
	DNode *it;
	daoint i, j;

	for(i=0; i<events->size; i++){
		DaoTaskEvent *event = (DaoTaskEvent*) events->items.pVoid[i];
		DaoFuture *future = event->future;
		DaoObject *actor = future->actor;
		DaoChannel *channel = event->channel;
		DaoChannel *closed = NULL;
		DaoChannel *chselect = NULL;
		DaoFuture *futselect = NULL;
		DaoValue *selected = NULL;
		DaoValue *message = NULL;
		int type = event->type;

		if( event->state == DAO_EVENT_WAIT && future->precond != NULL ){
			if( future->precond->state != DAO_CALL_FINISHED ) goto MoveToWaiting;
		}
		switch( event->type ){
		case DAO_EVENT_WAIT_SENDING :
			if( channel->buffer->size >= channel->cap ){
				if( event->state == DAO_EVENT_WAIT ){
					DaoChannel_ActivateEvent( channel, DAO_EVENT_WAIT_RECEIVING );
					DaoChannel_ActivateEvent( channel, DAO_EVENT_WAIT_SELECT );
					goto MoveToWaiting;
				}
			}
			event->type = DAO_EVENT_RESUME_TASKLET;
			break;
		case DAO_EVENT_WAIT_RECEIVING :
			if( channel->buffer->size == 0 ){
				if( channel->cap > 0 && event->state == DAO_EVENT_WAIT ){
					DaoChannel_ActivateEvent( channel, DAO_EVENT_WAIT_SENDING );
					goto MoveToWaiting;
				}
				message = dao_none_value;
			}else{
				message = channel->buffer->items.pValue[0];
			}
			GC_ShiftRC( message, event->message );
			event->message = message;
			event->auxiliary = channel->cap <= 0 && channel->buffer->size == 0;
			event->type = DAO_EVENT_RESUME_TASKLET;
			DArray_PopFront( channel->buffer );
			if( channel->buffer->size < channel->cap )
				DaoChannel_ActivateEvent( channel, DAO_EVENT_WAIT_SENDING );
			if( channel->buffer->size )
				DaoChannel_ActivateEvent( channel, DAO_EVENT_WAIT_RECEIVING );
			break;
		case DAO_EVENT_WAIT_SELECT :
			message = dao_none_value;
			for(it=DaoMap_First(event->selects); it; it=DaoMap_Next(event->selects,it)){
				if( DaoValue_CheckCtype( it->key.pValue, dao_type_channel ) ){
					DaoChannel *chan = (DaoChannel*) it->key.pValue;
					if( chan->buffer->size > 0 ){
						chselect = chan;
						selected = it->key.pValue;
						message = chan->buffer->items.pValue[0];
						closed = NULL;
						break;
					}else if( chan->cap == 0 ){
						closed = chan;
					}
				}else{
					DaoFuture *fut = (DaoFuture*) it->key.pValue;
					if( fut->state == DAO_CALL_FINISHED ){
						futselect = fut;
						selected = it->key.pValue;
						message = fut->value;
						break;
					}
				}
			}
			if( selected == NULL ) selected = (DaoValue*) closed;
			if( event->state == DAO_EVENT_WAIT && event->selects->items->size ){
				if( selected == NULL ) goto MoveToWaiting;
			}

			GC_ShiftRC( message, event->message );
			GC_ShiftRC( selected, event->selected );
			event->message = message;
			event->selected = selected;
			event->auxiliary = event->selects->items->size == 0;
			event->type = DAO_EVENT_RESUME_TASKLET;
			/* change status to not finished: */
			if( chselect != NULL || futselect != NULL ) event->auxiliary = 0;
			if( chselect ){
				DArray_PopFront( chselect->buffer );
				if( chselect->buffer->size < chselect->cap )
					DaoChannel_ActivateEvent( chselect, DAO_EVENT_WAIT_SENDING );
				if( chselect->buffer->size )
					DaoChannel_ActivateEvent( chselect, DAO_EVENT_WAIT_SELECT );
			}
			if( futselect != NULL || closed != NULL ){
				void *key = futselect ? (void*)futselect : (void*)closed;
				DMap_Erase( event->selects->items, key );
			}
			break;
		default: break;
		}
		if( actor ){
			DNode *it = DMap_Find( active, actor->rootObject );
			if( actor->rootObject->isAsync ){
				if( it && it->value.pVoid != (void*) future ) continue;
			}else if( it ){
				continue;
			}
		}
		if( future->process && DMap_Find( active, future->process ) ) continue;
		DArray_Erase( events, i, 1 );
		DMap_Erase( pending, event );
		if( actor ){
			void *value = actor->rootObject->isAsync ? future : NULL;
			DMap_Insert( active, actor->rootObject, value );
		}
		if( future->process ){
			DMap_Insert( active, future->process, NULL );
			future->process->active = 1;
		}

		GC_ShiftRC( event->message, future->message );
		GC_ShiftRC( event->selected, future->selected );
		future->message = event->message;
		future->selected = event->selected;
		future->aux1 = event->auxiliary;
		future->timeout = event->timeout;

		GC_IncRC( future ); /* To be decreased at the end of tasklet; */
		DaoCallServer_CacheEvent( event );
		return future;
MoveToWaiting:
		if( event->expiring >= 0.0 && event->expiring < MIN_TIME ) continue;
		if( event->expiring >= MIN_TIME ){
			DaoComplex com = {DAO_COMPLEX,0,0,0,1,{0.0,0.0}};
			com.value.real = event->expiring;
			DMap_Insert( server->waitings, & com, event );
			DCondVar_Signal( & server->condv2 );
		}else{
			DArray_Append( server->events2, event );
		}
		DArray_Erase( server->events, i, 1 );
		i -= 1;
	}
	return NULL;
}
static void DaoCallThread_Run( DaoCallThread *self )
{
	DaoCallServer *server = daoCallServer;
	double wt = 0.001;
	daoint i, count, timeout;

	self->thdData = DThread_GetSpecific();
	if( self->taskFunc ) self->taskFunc( self->taskParam );
	while(1){
		DaoProcess *process = NULL;
		DaoFuture *future = NULL;
		DThreadTask function = NULL;
		void *parameter = NULL;

		if( self->thdData != NULL ) self->thdData->state = 0;
		DMutex_Lock( & server->mutex );
		server->idle += 1;
		while( server->pending->size == (server->events2->size + server->waitings->size) ){
			//printf( "%p %i %i %i %i\n", self, server->events->size, server->pending->size, server->events2->size, server->waitings->size );
			if( server->finishing && server->idle == server->total ){
				if( (server->events2->size + server->waitings->size) == 0 ) break;
			}
			timeout = DCondVar_TimedWait( & server->condv, & server->mutex, wt );
		}
		for(i=0; i<server->parameters->size; ++i){
			void *param = server->parameters->items.pVoid[i];
			if( DMap_Find( server->active, param ) ) continue;
			DMap_Insert( server->active, param, NULL );
			function = (DThreadTask) server->functions->items.pVoid[i];
			parameter = param;
			DArray_Erase( server->functions, i, 1 );
			DArray_Erase( server->parameters, i, 1 );
			DMap_Erase( server->pending, parameter );
			server->idle -= 1;
			break;
		}
		DMutex_Unlock( & server->mutex );

		if( function ){
			(*function)( parameter );
			DMutex_Lock( & server->mutex );
			DMap_Erase( server->active, parameter );
			DMutex_Unlock( & server->mutex );
			continue;
		}

		if( server->pending->size == 0 && server->finishing && server->idle == server->total ) break;

		DMutex_Lock( & server->mutex );
		server->idle -= 1;
		future = DaoCallServer_GetNextFuture();
		DMutex_Unlock( & server->mutex );

		if( future == NULL ) continue;

		process = future->process;
		if( process == NULL ){
			GC_DecRC( future );
			continue;
		}

		count = process->exceptions->size;
		future->state = DAO_CALL_RUNNING;
		DaoProcess_InterceptReturnValue( process );
		DaoProcess_Execute( process );
		if( process->exceptions->size > count ) DaoProcess_PrintException( process, 1 );

		if( future->actor ){
			int erase = 1;
			DMutex_Lock( & server->mutex );
			if( future->actor->rootObject->isAsync ){
				erase = process->status == DAO_PROCESS_FINISHED;
			}
			if( erase ) DMap_Erase( server->active, future->actor->rootObject );
			DMutex_Unlock( & server->mutex );
		}
		DMutex_Lock( & server->mutex );
		DMap_Erase( server->active, process );
		process->active = 0;
		DMutex_Unlock( & server->mutex );

		DaoProcess_ReturnFutureValue( process, future );
		if( future->state == DAO_CALL_FINISHED ) DaoFuture_ActivateEvent( future );
		GC_DecRC( future );
	}
	DMutex_Lock( & server->mutex );
	server->stopped += 1;
	DMutex_Unlock( & server->mutex );
}
void DaoCallServer_Join()
{
	DCondVar condv;
	if( daoCallServer == NULL ) return;
	DCondVar_Init( & condv );
	DMutex_Lock( & daoCallServer->mutex );
	while( daoCallServer->pending->size || daoCallServer->idle != daoCallServer->total ){
		DCondVar_TimedWait( & condv, & daoCallServer->mutex, 0.01 );
	}
	DMutex_Unlock( & daoCallServer->mutex );
	DCondVar_Destroy( & condv );
}
void DaoCallServer_Stop()
{
	DCondVar condv;
	DaoCallThread *calth;
	if( daoCallServer == NULL ) return;
	DCondVar_Init( & condv );
	daoCallServer->finishing = 1;

	calth = DaoCallThread_New( NULL, NULL );
	DMutex_Lock( & daoCallServer->mutex );
	daoCallServer->total += 1;
	DMutex_Unlock( & daoCallServer->mutex );

	DaoCallThread_Run( calth );  /* process tasks in the main thread; */

	DMutex_Lock( & daoCallServer->mutex );
	while( daoCallServer->stopped != daoCallServer->total || daoCallServer->timing ){
		DCondVar_TimedWait( & condv, & daoCallServer->mutex, 0.01 );
	}
	DMutex_Unlock( & daoCallServer->mutex );

	DCondVar_Destroy( & condv );
	DaoCallThread_Delete( calth );
	DaoCallServer_Delete( daoCallServer );
	daoCallServer = NULL;
}






static int DaoType_CheckPrimitiveType( DaoType *self )
{
	daoint i;

	if( self == NULL || self->tid == DAO_NONE ) return 0;
	if( self->tid <= DAO_ARRAY ) return 1;
	if( self->tid > DAO_TUPLE && self->tid != DAO_VARIANT ) return 0;

	if( self->tid != DAO_TUPLE && (self->nested == NULL || self->nested->size == 0) ) return 0;
	for(i=0; i<self->nested->size; ++i){
		DaoType *type = self->nested->items.pType[i];
		if( type == NULL ) return 0;
		if( type->tid == DAO_PAR_NAMED ) type = (DaoType*) type->aux;
		if( DaoType_CheckPrimitiveType( type ) == 0 ) return 0;
	}
	return 1;
}
static DaoValue* DaoValue_DeepCopy( DaoValue *self )
{
	DNode *it;
	daoint i;
	if( self == NULL ) return NULL;
	if( self->type <= DAO_ENUM ) return self; /* simple types will be copied at use; */
	if( self->type == DAO_ARRAY ) return (DaoValue*) DaoArray_Copy( (DaoArray*) self );
	if( self->type == DAO_LIST ){
		DaoList *list = (DaoList*) self;
		DaoList *copy = DaoList_New();
		GC_ShiftRC( list->unitype, copy->unitype );
		copy->unitype = list->unitype;
		for(i=0; i<list->items.size; ++i){
			DaoValue *value = DaoValue_DeepCopy( list->items.items.pValue[i] );
			DaoList_Append( copy, value );
		}
		return (DaoValue*) copy;
	}else if( self->type == DAO_MAP ){
		DaoMap *map = (DaoMap*) self;
		DaoMap *copy = DaoMap_New( map->items->hashing );
		GC_ShiftRC( map->unitype, copy->unitype );
		copy->unitype = map->unitype;
		for(it=DMap_First(map->items); it; it=DMap_Next(map->items,it)){
			DaoValue *key = DaoValue_DeepCopy( it->key.pValue );
			DaoValue *value = DaoValue_DeepCopy( it->value.pValue );
			DaoMap_Insert( copy, key, value );
		}
		return (DaoValue*) copy;
	}else if( self->type == DAO_TUPLE ){
		DaoTuple *tuple = (DaoTuple*) self;
		DaoTuple *copy = DaoTuple_New( tuple->size );
		GC_ShiftRC( tuple->unitype, copy->unitype );
		copy->unitype = tuple->unitype;
		for(i=0; i<tuple->size; ++i){
			DaoValue *value = DaoValue_DeepCopy( tuple->items[i] );
			DaoTuple_SetItem( copy, value, i );
		}
		return (DaoValue*) copy;
	}
	return NULL;
}


static void CHANNEL_SetCap( DaoChannel *self, DaoValue *value, DaoProcess *proc )
{
	self->cap = value->xInteger.value;
	if( self->cap > 0 ) return;
	self->cap = 1;
	DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "channel capacity must be greater than 0" );
}
static void CHANNEL_New( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoType *retype = DaoProcess_GetReturnType( proc );
	DaoChannel *self = DaoChannel_New( retype, 0 );
	CHANNEL_SetCap( self, par[0], proc );
	if( DaoType_CheckPrimitiveType( retype->nested->items.pType[0] ) == 0 ){
		DString *s = DString_New(1);
		DString_AppendMBS( s, "data type " );
		DString_Append( s, retype->nested->items.pType[0]->name );
		DString_AppendMBS( s, " is not supported for channel" );
		DaoProcess_RaiseException( proc, DAO_ERROR, s->mbs );
		DString_Delete( s );
	}
	DaoProcess_PutValue( proc, (DaoValue*) self );
	DaoCallServer_TryInit( mainVmSpace );
}
static void CHANNEL_Buffer( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoChannel *self = (DaoChannel*) par[0];
	DaoProcess_PutInteger( proc, self->buffer->size );
}
static void CHANNEL_Cap( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoChannel *self = (DaoChannel*) par[0];
	daoint i;

	DaoProcess_PutInteger( proc, self->cap );
	if( N == 1 ) return;

	self->cap = par[1]->xInteger.value;
	if( self->cap > 0 ) return;
	self->cap = 0;

	/* Closing the channel: */
	DMutex_Lock( & daoCallServer->mutex );
	DaoChannel_ActivateEvent( self, DAO_EVENT_WAIT_RECEIVING );
	DaoChannel_ActivateEvent( self, DAO_EVENT_WAIT_SELECT );
	DCondVar_Signal( & daoCallServer->condv );
	DMutex_Unlock( & daoCallServer->mutex );
}
static int DaoProcess_CheckCB( DaoProcess *self, const char *message )
{
	if( self->depth == 0 ) return 0;
	DaoProcess_RaiseException( self, DAO_ERROR, message );
	return 1;
}
static void CHANNEL_Send( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoValue *data;
	DaoFuture *future = DaoProcess_GetInitFuture( proc );
	DaoChannel *self = (DaoChannel*) par[0];
	float timeout = par[2]->xFloat.value;

	if( DaoProcess_CheckCB( proc, "cannot send/block inside code section method" ) ) return;
	if( self->cap <= 0 ){
		DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "channel is closed" );
		return;
	}

	data = DaoValue_DeepCopy( par[1] );
	if( data == NULL ){
		DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "invalid data for the channel" );
		return;
	}

	//printf( "CHANNEL_Send: %p\n", event );
	DMutex_Lock( & daoCallServer->mutex );
	DArray_Append( self->buffer, data );
	DaoChannel_ActivateEvent( self, DAO_EVENT_WAIT_RECEIVING );
	DaoChannel_ActivateEvent( self, DAO_EVENT_WAIT_SELECT );
	DCondVar_Signal( & daoCallServer->condv );
	DMutex_Unlock( & daoCallServer->mutex );

	if( self->buffer->size >= self->cap ){
		DaoTaskEvent *event = DaoCallServer_MakeEvent();
		DaoTaskEvent_Init( event, DAO_EVENT_WAIT_SENDING, DAO_EVENT_WAIT, future, self );
		proc->status = DAO_PROCESS_SUSPENDED;
		proc->pauseType = DAO_PAUSE_CHANNEL_SEND;
		DaoCallServer_AddTimedWait( proc, event, timeout );
	}
}
static void CHANNEL_Receive( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoTaskEvent *event = NULL;
	DaoFuture *future = DaoProcess_GetInitFuture( proc );
	DaoChannel *self = (DaoChannel*) par[0];
	float timeout = par[1]->xFloat.value;

	if( DaoProcess_CheckCB( proc, "cannot receive/block inside code section method" ) ) return;

	event = DaoCallServer_MakeEvent();
	DaoTaskEvent_Init( event, DAO_EVENT_WAIT_RECEIVING, DAO_EVENT_WAIT, future, self );
	proc->status = DAO_PROCESS_SUSPENDED;
	proc->pauseType = DAO_PAUSE_CHANNEL_RECEIVE;
	DaoCallServer_AddTimedWait( proc, event, timeout );

	/* Message may have been sent before this call: */
	if( self->buffer->size ){
		DMutex_Lock( & daoCallServer->mutex );
		DaoChannel_ActivateEvent( self, DAO_EVENT_WAIT_RECEIVING );
		DCondVar_Signal( & daoCallServer->condv );
		DMutex_Unlock( & daoCallServer->mutex );
	}
}

static DaoFuncItem channelMeths[] =
{
	{ CHANNEL_New,      "channel<@V>( cap = 1 )" },
	{ CHANNEL_Buffer,   "buffer( self :channel<@V> ) => int" },
	{ CHANNEL_Cap,      "cap( self :channel<@V> ) => int" },
	{ CHANNEL_Cap,      "cap( self :channel<@V>, cap :int ) =>int" },
	{ CHANNEL_Send,     "send( self :channel<@V>, data :@V, timeout :float = -1 ) => int" },
	{ CHANNEL_Receive,  "receive( self :channel<@V>, timeout :float = -1 ) => tuple<data :@V|none, status :enum<received,timeout,finished>>" },
	{ NULL, NULL }
};
static void DaoChannel_Delete( DaoChannel *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DArray_Delete( self->buffer );
	dao_free( self );
}
static void DaoChannel_GetGCFields( void *p, DArray *vs, DArray *arrays, DArray *ms, int rm )
{
	DaoChannel *self = (DaoChannel*) p;
	DArray_Append( arrays, self->buffer );
}

DaoTypeBase channelTyper =
{
	"channel<@V=none>", NULL, NULL, (DaoFuncItem*) channelMeths, {0}, {0},
	(FuncPtrDel) DaoChannel_Delete, DaoChannel_GetGCFields
};





static void FUTURE_Value( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoFuture *self = (DaoFuture*) par[0];
	if( self->state == DAO_CALL_FINISHED ){
		DaoProcess_PutValue( proc, self->value );
		return;
	}
	if( DaoProcess_CheckCB( proc, "cannot block inside code section method" ) ) return;
	proc->status = DAO_PROCESS_SUSPENDED;
	proc->pauseType = DAO_PAUSE_FUTURE_VALUE;
	DaoCallServer_AddWait( proc, self, -1 );
}
static void FUTURE_Wait( DaoProcess *proc, DaoValue *par[], int N )
{
	DaoFuture *self = (DaoFuture*) par[0];
	float timeout = par[1]->xFloat.value;
	DaoProcess_PutInteger( proc, self->state == DAO_CALL_FINISHED );
	if( DaoProcess_CheckCB( proc, "cannot block inside code section method" ) ) return;
	if( self->state == DAO_CALL_FINISHED || timeout == 0 ) return;
	proc->status = DAO_PROCESS_SUSPENDED;
	proc->pauseType = DAO_PAUSE_FUTURE_WAIT;
	DaoCallServer_AddWait( proc, self, timeout );
}
static DaoFuncItem futureMeths[] =
{
	{ FUTURE_Value,   "value( self : future<@V> )=>@V" },
	{ FUTURE_Wait,    "wait( self : future<@V>, timeout : float = -1 )=>int" },
	{ NULL, NULL }
};
static void DaoFuture_Delete( DaoFuture *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	GC_DecRC( self->value );
	GC_DecRC( self->actor );
	GC_DecRC( self->message );
	GC_DecRC( self->selected );
	GC_DecRC( self->process );
	GC_DecRC( self->precond );
	dao_free( self );
}
static void DaoFuture_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	DaoFuture *self = (DaoFuture*) p;
	if( self->value ) DArray_Append( values, self->value );
	if( self->actor ) DArray_Append( values, self->actor );
	if( self->message ) DArray_Append( values, self->message );
	if( self->selected ) DArray_Append( values, self->selected );
	if( self->process ) DArray_Append( values, self->process );
	if( self->precond ) DArray_Append( values, self->precond );
	if( remove ){
		self->value = NULL;
		self->actor = NULL;
		self->message = NULL;
		self->selected = NULL;
		self->process = NULL;
		self->precond = NULL;
	}
}

DaoTypeBase futureTyper =
{
	"future<@V=none>", NULL, NULL, (DaoFuncItem*) futureMeths, {0}, {0},
	(FuncPtrDel) DaoFuture_Delete, DaoFuture_GetGCFields
};




void DaoMT_Select( DaoProcess *proc, DaoValue *par[], int n )
{
	DNode *it;
	DaoTaskEvent *event = NULL;
	DaoFuture *future = DaoProcess_GetInitFuture( proc );
	DaoMap *selects = (DaoMap*) par[0];
	float timeout = par[1]->xFloat.value;

	if( DaoProcess_CheckCB( proc, "cannot select/block inside code section method" ) ) return;

	for(it=DaoMap_First(selects); it; it=DaoMap_Next(selects,it)){
		DaoValue *value = it->key.pValue;
		int isfut = DaoValue_CheckCtype( value, dao_type_future );
		int ischan = DaoValue_CheckCtype( value, dao_type_channel );
		if( isfut == 0 && ischan == 0 ){
			DaoProcess_RaiseException( proc, DAO_ERROR_PARAM, "invalid type selection" );
			return;
		}
	}

	event = DaoCallServer_MakeEvent();
	DaoTaskEvent_Init( event, DAO_EVENT_WAIT_SELECT, DAO_EVENT_WAIT, future, NULL );
	GC_ShiftRC( selects, event->selects );
	event->selects = selects;
	proc->status = DAO_PROCESS_SUSPENDED;
	proc->pauseType = DAO_PAUSE_CHANFUT_SELECT;
	DaoCallServer_AddTimedWait( proc, event, timeout );

	/* Message may have been sent before this call: */
	DMutex_Lock( & daoCallServer->mutex );
	DaoChannel_ActivateEvent( NULL, DAO_EVENT_WAIT_SELECT );
	DCondVar_Signal( & daoCallServer->condv );
	DMutex_Unlock( & daoCallServer->mutex );
}


#endif
