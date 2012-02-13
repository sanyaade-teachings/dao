
#include<assert.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<stack>

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h" // "llvm/Support/TargetSelect.h" cause error!
#include "llvm/Target/TargetData.h"
#include "llvm/PassManager.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/IRBuilder.h"

#include "daoJIT.h"

LLVMContext     *llvm_context = NULL;
Module          *llvm_module = NULL;
ExecutionEngine *llvm_exe_engine = NULL;
FunctionPassManager *llvm_func_optimizer = NULL;

PointerType *dao_number_types[DAO_DOUBLE] = {0};
Type *cxx_number_types[DAO_DOUBLE] = {0};

Type *int1_type = NULL;
Type *int8_type = NULL;
Type *int16_type = NULL;
Type *int32_type = NULL;
Type *int64_type = NULL;
Type *daoint_type = NULL; // 32 or 64 bits
Type *float_type = NULL;
Type *double_type = NULL;
Type *size_t_type = NULL;
VectorType *int8_vector2 = NULL;

Type *void_type = NULL; // void

StructType *dstring_type = NULL; // DString
PointerType *dstring_type_p = NULL; // DString*
PointerType *dstring_type_pp = NULL; // DString**

StructType *darray_value_type = NULL; // DArray<DaoValue*>
PointerType *darray_value_type_p = NULL; // DArray<DaoValue*>*

StructType *darray_class_type = NULL; // DArray<DaoClass*>
PointerType *darray_class_type_p = NULL; // DArray<DaoClass*>*

StructType *darray_namespace_type = NULL; // DArray<DaoNamespace*>
PointerType *darray_namespace_type_p = NULL; // DArray<DaoNamespace*>*

StructType *dao_value_type = NULL; // DaoValue
StructType *dao_integer_type = NULL; // DaoInteger
StructType *dao_float_type = NULL; // DaoFloat
StructType *dao_double_type = NULL; // DaoDouble
StructType *dao_string_type = NULL; // DaoString
StructType *dao_enum_type = NULL; // DaoEnum
StructType *dao_list_type = NULL; // DaoList
StructType *dao_tuple_type = NULL; // DaoTuple
StructType *dao_class_type = NULL; // DaoClass
StructType *dao_object_type = NULL; // DaoObject
StructType *dao_namespace_type = NULL; // DaoNamespace
StructType *dao_type_type = NULL; // DaoType

PointerType *void_type_p = NULL; // i8*

PointerType *dao_value_type_p = NULL; // DaoValue*
PointerType *dao_integer_type_p = NULL; // DaoInteger*
PointerType *dao_float_type_p = NULL; // DaoFloat*
PointerType *dao_double_type_p = NULL; // DaoDouble*
PointerType *dao_string_type_p = NULL; // DaoString*
PointerType *dao_enum_type_p = NULL; // DaoEnum*
PointerType *dao_list_type_p = NULL; // DaoList*
PointerType *dao_tuple_type_p = NULL; // DaoTuple*
PointerType *dao_class_type_p = NULL; // DaoClass*
PointerType *dao_object_type_p = NULL; // DaoObject*
PointerType *dao_namespace_type_p = NULL; // DaoNamespace*
PointerType *dao_type_type_p = NULL; // DaoType*

PointerType *dao_value_type_pp = NULL; // DaoValue**
PointerType *dao_string_type_pp = NULL; // DaoString**
PointerType *dao_enum_type_pp = NULL; // DaoEnum**
PointerType *dao_list_type_pp = NULL; // DaoList**
PointerType *dao_tuple_type_pp = NULL; // DaoTuple**
PointerType *dao_class_type_pp = NULL; // DaoClass**
PointerType *dao_object_type_pp = NULL; // DaoObject**
PointerType *dao_namespace_type_pp = NULL; // DaoNamespace**

ArrayType *dao_value_ptr_array_type = NULL; // DaoValue*[]
PointerType *dao_value_ptr_array_type_p = NULL; // DaoValue*[]*

ArrayType *dao_type_ptr_array_type = NULL; // DaoType*[]
PointerType *dao_type_ptr_array_type_p = NULL; // DaoType*[]*

ArrayType *dao_class_ptr_array_type = NULL; // DaoClass*[]
PointerType *dao_class_ptr_array_type_p = NULL; // DaoClass*[]*

ArrayType *dao_namespace_ptr_array_type = NULL; // DaoNamespace*[]
PointerType *dao_namespace_ptr_array_type_p = NULL; // DaoNamespace*[]*

StructType *dao_jit_call_data_type = NULL; // DaoJitCallData
PointerType *dao_jit_call_data_type_p = NULL; // DaoJitCallData*

int dao_opcode_compilable[ DVM_NULL ];

Function *dao_debug_function = NULL;

Function *dao_pow_double = NULL;
Function *dao_abs_double = NULL;
Function *dao_acos_double = NULL;
Function *dao_asin_double = NULL;
Function *dao_atan_double = NULL;
Function *dao_ceil_double = NULL;
Function *dao_cos_double = NULL;
Function *dao_cosh_double = NULL;
Function *dao_exp_double = NULL;
Function *dao_floor_double = NULL;
Function *dao_log_double = NULL;
Function *dao_rand_double = NULL;
Function *dao_sin_double = NULL;
Function *dao_sinh_double = NULL;
Function *dao_sqrt_double = NULL;
Function *dao_tan_double = NULL;
Function *dao_tanh_double = NULL;

Function *dao_string_move = NULL;
Function *dao_string_add = NULL;
Function *dao_string_lt = NULL;
Function *dao_string_le = NULL;
Function *dao_string_eq = NULL;
Function *dao_string_ne = NULL;
Function *dao_geti_si = NULL;
Function *dao_seti_sii = NULL;
Function *dao_seti_li = NULL;
Function *dao_seti_ti = NULL;
Function *dao_setf_t = NULL;

Function *dao_move_pp = NULL;
Function *dao_value_copy = NULL;
Function *dao_value_move = NULL;

FunctionType *dao_jit_function_type = NULL;

extern "C"{

void dao_debug( void *p )
{
	printf( "debug: %p\n", p );
	fflush( stdout );
}

double dao_rand( double max ){ return max * rand() / (RAND_MAX+1.0); }


void DaoJIT_MOVE_PP( DaoValue *dA, DaoValue **dC )
{
	GC_ShiftRC( dA, *dC );
	*dC = dA;
}
void DaoJIT_MOVE_SS( DaoValue *dA, DaoValue **dC2 )
{
	DaoValue *dC = *dC2;
	if( dC && dC->type == DAO_STRING ){
		DString_Assign( dC->xString.data, dA->xString.data );
	}else{
		DaoString *S = DaoString_Copy( (DaoString*) dA );
		GC_ShiftRC( S, dC );
		*dC2 = (DaoValue*) S;
	}
}
void DaoJIT_ADD_SS( DaoValue *dA, DaoValue *dB, DaoValue **dC2 )
{
	DaoValue *dC = *dC2;
	if( dA == dC ){
		DString_Append( dA->xString.data, dB->xString.data );
	}else if( dB == dC ){
		DString_Insert( dB->xString.data, dA->xString.data, 0, 0, 0 );
	}else{
		if( dC && dC->type == DAO_STRING ){
			DString_Assign( dC->xString.data, dA->xString.data );
		}else{
			DaoString *S = DaoString_Copy( (DaoString*) dA );
			GC_ShiftRC( S, dC );
			*dC2 = (DaoValue*) S;
		}
		DString_Append( dC->xString.data, dB->xString.data );
	}
}
void DaoJIT_ADD_LT( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.data, dB->xString.data ) <0);
}
void DaoJIT_ADD_LE( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.data, dB->xString.data ) <=0);
}
void DaoJIT_ADD_EQ( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.data, dB->xString.data ) ==0);
}
void DaoJIT_ADD_NE( DaoValue *dA, DaoValue *dB, DaoValue *dC )
{
	dC->xInteger.value = (DString_Compare( dA->xString.data, dB->xString.data ) !=0);
}
int DaoJIT_GETI_SI( DaoValue *dA, DaoValue *dB, DaoValue *dC, int vmc )
{
	wchar_t *wcs = dA->xString.data->wcs;
	char *mbs = dA->xString.data->mbs;
	daoint id = dB->xInteger.value;
	if( id <0 ) id += dA->xString.data->size;
	if( id <0 || id >= dA->xString.data->size ) return (DAO_ERROR_INDEX<<16)|vmc;
	dC->xInteger.value = mbs ? mbs[id] : wcs[id];
	return 0;
}
int DaoJIT_SETI_SII( DaoValue *dA, DaoValue *dB, DaoValue *dC, int vmc )
{
	wchar_t *wcs = dC->xString.data->wcs;
	char *mbs = dC->xString.data->mbs;
	daoint id = dB->xInteger.value;
	daoint ch = dA->xInteger.value;
	if( id <0 ) id += dC->xString.data->size;
	if( id <0 || id >= dC->xString.data->size ) return (DAO_ERROR_INDEX<<16)|vmc;
	if( mbs ) mbs[id] = ch; else wcs[id] = ch;
	return 0;
}
int DaoJIT_SETI_LI( DaoValue *dA, DaoValue *dB, DaoValue *dC, int vmc )
{
	DaoList *list = (DaoList*) dC;
	daoint id = dB->xInteger.value;
	if( dC->xNone.trait & DAO_VALUE_CONST ) return (DAO_ERROR<<16)|vmc;
	if( id <0 ) id += list->items.size;
	if( id <0 || id >= list->items.size ) return (DAO_ERROR_INDEX<<16)|vmc;
	GC_ShiftRC( dA, list->items.items.pValue[id] );
	list->items.items.pValue[id] = dA;
	return 0;
}
int DaoJIT_SETI_TI( DaoValue *dA, DaoValue *dB, DaoValue *dC, int vmc )
{
	DaoTuple *tuple = (DaoTuple*) dC;
	DaoType *type = NULL;
	daoint id = dB->xInteger.value;
	if( id <0 || id >= tuple->size ) return (DAO_ERROR_INDEX<<16)|vmc;
	type = tuple->unitype->nested->items.pType[id];
	if( type->tid == DAO_PAR_NAMED ) type = & type->aux->xType;
	if( DaoValue_Move( dA, tuple->items + id, type ) ==0 ) return (DAO_ERROR_VALUE<<16)|vmc;
	return 0;
}
int DaoJIT_SETF_T( DaoValue *dA, int id, DaoValue *dC, int vmc )
{
	DaoTuple *tuple = (DaoTuple*) dC;
	GC_ShiftRC( dA, tuple->items[id] );
	tuple->items[id] = dA;
	return 0;
}
}

#define CHECK_MODE 8

void DaoJIT_Init( DaoVmSpace *vms, DaoJIT *jit )
{
	int i;
	memset( dao_opcode_compilable, 0, DVM_NULL*sizeof(int) );
	for(i=DVM_MOVE_II; i<=DVM_MOVE_DD; i++) dao_opcode_compilable[i] = 1;
	for(i=DVM_ADD_III; i<=DVM_BITRIT_DNN; i++) dao_opcode_compilable[i] = 1;
	for(i=DVM_ADD_SS; i<=DVM_NE_SS; i++) dao_opcode_compilable[i] = 1;
	for(i=DVM_GETF_KCI; i<=DVM_SETF_OVDD; i++) dao_opcode_compilable[i] = 1;
	for(i=DVM_GETI_TI; i<=DVM_SETF_TSS; i++) dao_opcode_compilable[i] = 1;
	for(i=DVM_GETI_LII; i<=DVM_GETI_LDI; i++) dao_opcode_compilable[i] = CHECK_MODE;
	for(i=DVM_SETI_LIII; i<=DVM_SETI_LDID; i++) dao_opcode_compilable[i] = 1;
	for(i=DVM_DATA_I; i<=DVM_SETVG_DD; i++) dao_opcode_compilable[i] = 1;
	dao_opcode_compilable[ DVM_NOP ] = 1;
	dao_opcode_compilable[ DVM_GOTO ] = 1;
	dao_opcode_compilable[ DVM_SWITCH ] = 1;
	dao_opcode_compilable[ DVM_CASE ] = 1;
	dao_opcode_compilable[ DVM_TRY ] = 1;
	dao_opcode_compilable[ DVM_RAISE ] = 1;
	dao_opcode_compilable[ DVM_CATCH ] = 1;
	dao_opcode_compilable[ DVM_TEST ] = 1;
	dao_opcode_compilable[ DVM_TEST_I ] = 1;
	dao_opcode_compilable[ DVM_TEST_F ] = 1;
	dao_opcode_compilable[ DVM_TEST_D ] = 1;
	dao_opcode_compilable[ DVM_GETI_SI ] = 1;
	dao_opcode_compilable[ DVM_SETI_SII ] = 1;
	dao_opcode_compilable[ DVM_GETI_LI ] = CHECK_MODE;
	dao_opcode_compilable[ DVM_SETI_LI ] = 1;
	dao_opcode_compilable[ DVM_SETI_TI ] = 1;
	dao_opcode_compilable[ DVM_SETF_TPP ] = 1;
	dao_opcode_compilable[ DVM_MOVE_PP ] = 1;
	dao_opcode_compilable[ DVM_MOVE_SS ] = 1;

	//dao_opcode_compilable[ DVM_AND_III ] = 0;
	//dao_opcode_compilable[ DVM_OR_III ] = 0;
	//dao_opcode_compilable[ DVM_LT_III ] = 0;
	//dao_opcode_compilable[ DVM_LT_FFF ] = 0;

	// TODO: GETCX, GETVX, swith, complex, string, array, tuple, list etc.

	printf( "DaoJIT_Init()\n" );
	InitializeNativeTarget();
	llvm_context = new LLVMContext();
	llvm_module = new Module("DaoJIT", *llvm_context);

	int size_t_bits = CHAR_BIT * sizeof(size_t) - 1;

	Type *string_size_type = Type::getIntNTy( *llvm_context, size_t_bits - 1 );

	int1_type = Type::getInt1Ty( *llvm_context );
	int8_type = Type::getInt8Ty( *llvm_context );
	int16_type = Type::getInt16Ty( *llvm_context );
	int32_type = Type::getInt32Ty( *llvm_context );
	int64_type = Type::getInt64Ty( *llvm_context );
	int8_vector2 = VectorType::get( int8_type, 2 );

	daoint_type = int32_type;
	if( sizeof(void*) == 8 ) daoint_type = Type::getInt64Ty( *llvm_context );
	float_type = Type::getFloatTy( *llvm_context );
	double_type = Type::getDoubleTy( *llvm_context );

	cxx_number_types[DAO_INTEGER - DAO_INTEGER] = daoint_type;
	cxx_number_types[DAO_FLOAT   - DAO_INTEGER] = float_type;
	cxx_number_types[DAO_DOUBLE  - DAO_INTEGER] = double_type;

	size_t_type = daoint_type;

	void_type = Type::getVoidTy( *llvm_context );
	void_type_p = PointerType::getUnqual( int8_type );

	std::vector<Type*> field_types( 6, string_size_type );
	field_types[1] = int1_type;
	field_types[3] = int1_type;
	field_types[4] = PointerType::getUnqual( int8_type );
	field_types[5] = PointerType::getUnqual( int32_type );

	dstring_type = StructType::get( *llvm_context, field_types );
	dstring_type_p = PointerType::getUnqual( dstring_type );
	dstring_type_pp = PointerType::getUnqual( dstring_type_p );

	field_types.clear();
	field_types.resize( 4, int8_type );
	field_types.push_back( int32_type ); // refCount

	// type { i8, i8, i8, i8, i32 }
	dao_value_type = StructType::get( *llvm_context, field_types );
	dao_value_type_p = PointerType::getUnqual( dao_value_type );
	dao_value_type_pp = PointerType::getUnqual( dao_value_type_p );
	dao_value_ptr_array_type = ArrayType::get( dao_value_type_p, 0 );
	dao_value_ptr_array_type_p = PointerType::getUnqual( dao_value_ptr_array_type );

	// type { i8, i8, i8, i8, i32, daoint }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( daoint_type );
	dao_integer_type = StructType::get( *llvm_context, field_types );
	dao_integer_type_p = PointerType::getUnqual( dao_integer_type );
	//dao_integer_type_pp = PointerType::getUnqual( dao_integer_type_p );

	// type { i8, i8, i8, i8, i32, float }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( float_type );
	dao_float_type = StructType::get( *llvm_context, field_types );
	dao_float_type_p = PointerType::getUnqual( dao_float_type );
	//dao_float_type_pp = PointerType::getUnqual( dao_float_type_p );

	// type { i8, i8, i8, i8, i32, double }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( double_type );
	dao_double_type = StructType::get( *llvm_context, field_types );
	dao_double_type_p = PointerType::getUnqual( dao_double_type );
	//dao_double_type_pp = PointerType::getUnqual( dao_double_type_p );

	dao_number_types[DAO_INTEGER - DAO_INTEGER] = dao_integer_type_p;
	dao_number_types[DAO_FLOAT   - DAO_INTEGER] = dao_float_type_p;
	dao_number_types[DAO_DOUBLE  - DAO_INTEGER] = dao_double_type_p;

	// type { i8, i8, i8, i8, i32, DString* }
	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( dstring_type_p );
	dao_string_type = StructType::get( *llvm_context, field_types );
	dao_string_type_p = PointerType::getUnqual( dao_string_type );
	dao_string_type_pp = PointerType::getUnqual( dao_string_type_p );

	field_types.erase( field_types.begin()+5, field_types.end() );
	field_types.push_back( int32_type ); // cycRefCount

	// type { i8, i8, i8, i8, i32, i32, i32 }
	field_types.push_back( int32_type ); // value
	dao_enum_type = StructType::get( *llvm_context, field_types );
	dao_enum_type_p = PointerType::getUnqual( dao_enum_type );
	dao_enum_type_pp = PointerType::getUnqual( dao_enum_type_p );

	// type { i8, i8, i8, i8, i32, i32, i8 }
	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int8_type ); // value
	dao_type_type = StructType::get( *llvm_context, field_types );
	dao_type_type_p = PointerType::getUnqual( dao_type_type );
	dao_type_ptr_array_type = ArrayType::get( dao_type_type_p, 0 );
	dao_type_ptr_array_type_p = PointerType::getUnqual( dao_type_ptr_array_type );


	std::vector<Type*> array_types( 1, dao_value_ptr_array_type_p ); // items
	array_types.push_back( size_t_type ); // size
	darray_value_type = StructType::get( *llvm_context, array_types );
	darray_value_type_p = PointerType::getUnqual( darray_value_type );

	array_types[0] = dao_class_ptr_array_type_p;
	darray_class_type = StructType::get( *llvm_context, array_types );
	darray_class_type_p = PointerType::getUnqual( darray_class_type );

	array_types[0] = dao_namespace_ptr_array_type_p;
	darray_namespace_type = StructType::get( *llvm_context, array_types );
	darray_namespace_type_p = PointerType::getUnqual( darray_namespace_type );


	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( darray_value_type ); // items
	dao_list_type = StructType::get( *llvm_context, field_types );
	dao_list_type_p = PointerType::getUnqual( dao_list_type );
	dao_list_type_pp = PointerType::getUnqual( dao_list_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int32_type ); // size
	field_types.push_back( dao_type_type_p ); // unitype
	field_types.push_back( dao_value_ptr_array_type ); // items
	dao_tuple_type = StructType::get( *llvm_context, field_types );
	dao_tuple_type_p = PointerType::getUnqual( dao_tuple_type );
	dao_tuple_type_pp = PointerType::getUnqual( dao_tuple_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	for(i=0; i<2; i++) field_types.push_back( void_type_p );
	for(i=0; i<2; i++) field_types.push_back( darray_value_type_p );
	dao_class_type = StructType::get( *llvm_context, field_types );
	dao_class_type_p = PointerType::getUnqual( dao_class_type );
	dao_class_type_pp = PointerType::getUnqual( dao_class_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	field_types.push_back( int16_type ); // bit fields;
	field_types.push_back( int16_type ); // valueCount;
	field_types.push_back( dao_class_type_p ); // defClass;
	field_types.push_back( void_type_p ); // rootObject;
	field_types.push_back( dao_value_ptr_array_type_p ); // objValues;
	dao_object_type = StructType::get( *llvm_context, field_types );
	dao_object_type_p = PointerType::getUnqual( dao_object_type );
	dao_object_type_pp = PointerType::getUnqual( dao_object_type_p );

	field_types.erase( field_types.begin()+6, field_types.end() );
	for(i=0; i<2; i++) field_types.push_back( void_type_p );
	for(i=0; i<3; i++) field_types.push_back( darray_value_type_p );
	dao_namespace_type = StructType::get( *llvm_context, field_types );
	dao_namespace_type_p = PointerType::getUnqual( dao_namespace_type );
	dao_namespace_type_pp = PointerType::getUnqual( dao_namespace_type_p );

	
	std::vector<Type*> jitcd_types( 8, dao_value_ptr_array_type_p );
	jitcd_types.push_back( void_type_p );
	jitcd_types.push_back( darray_value_type_p );
	jitcd_types.push_back( darray_value_type_p );
	dao_jit_call_data_type = StructType::get( *llvm_context, jitcd_types );
	dao_jit_call_data_type_p = PointerType::getUnqual( dao_jit_call_data_type );


	// void (process*,routine*)
	std::vector<Type*> jitParams( 1, dao_jit_call_data_type_p );
	dao_jit_function_type = FunctionType::get( int32_type, jitParams, false );

	std::vector<Type*> value_params( 1, dao_value_type_p );
	value_params.push_back( dao_value_type_p );
	FunctionType *ft0 = FunctionType::get( void_type, value_params, false );
	dao_value_copy = Function::Create( ft0, Function::ExternalLinkage, "DaoValue_Copy", llvm_module );

	value_params.clear();
	value_params.push_back( dao_value_type );
	value_params.push_back( dao_value_type_p );
	value_params.push_back( dao_type_type_p );
	ft0 = FunctionType::get( int32_type, value_params, false );
	dao_value_move = Function::Create( ft0, Function::ExternalLinkage, "DaoValue_Move", llvm_module );

	std::vector<Type*> value2( 1, dao_value_type_p );
	value2.push_back( dao_value_type_pp );
	FunctionType *ft2 = FunctionType::get( void_type, value2, false );
	dao_string_move = Function::Create( ft2, Function::ExternalLinkage, "DaoJIT_MOVE_SS", llvm_module );
	dao_move_pp = Function::Create( ft2, Function::ExternalLinkage, "DaoJIT_MOVE_PP", llvm_module );

	std::vector<Type*> value3( 2, dao_value_type_p );
	value3.push_back( dao_value_type_pp );
	FunctionType *ft3 = FunctionType::get( void_type, value3, false );
	dao_string_add = Function::Create( ft3, Function::ExternalLinkage, "DaoJIT_ADD_SS", llvm_module );

	value3[2] = dao_value_type_p;
	ft3 = FunctionType::get( void_type, value3, false );
	dao_string_lt = Function::Create( ft3, Function::ExternalLinkage, "DaoJIT_ADD_LT", llvm_module );
	dao_string_le = Function::Create( ft3, Function::ExternalLinkage, "DaoJIT_ADD_LE", llvm_module );
	dao_string_eq = Function::Create( ft3, Function::ExternalLinkage, "DaoJIT_ADD_EQ", llvm_module );
	dao_string_ne = Function::Create( ft3, Function::ExternalLinkage, "DaoJIT_ADD_NE", llvm_module );

	value3.push_back( int32_type );
	FunctionType *ft4 = FunctionType::get( int32_type, value3, false );
	dao_geti_si = Function::Create( ft4, Function::ExternalLinkage, "DaoJIT_GETI_SI", llvm_module );
	dao_seti_sii = Function::Create( ft4, Function::ExternalLinkage, "DaoJIT_SETI_SII", llvm_module );
	dao_seti_li = Function::Create( ft4, Function::ExternalLinkage, "DaoJIT_SETI_LI", llvm_module );
	dao_seti_ti = Function::Create( ft4, Function::ExternalLinkage, "DaoJIT_SETI_TI", llvm_module );

	value3[1] = int32_type;
	ft4 = FunctionType::get( int32_type, value3, false );
	dao_setf_t = Function::Create( ft4, Function::ExternalLinkage, "DaoJIT_SETF_T", llvm_module );

	std::vector<Type*> double2( 2, double_type );
	FunctionType *funtype = FunctionType::get( double_type, double2, false );
	dao_pow_double = Function::Create( funtype, Function::ExternalLinkage, "pow", llvm_module );

	std::vector<Type*> double1( 1, double_type );
	std::vector<Type*> voidp1( 1, void_type_p );
	FunctionType *mathft = FunctionType::get( double_type, double1, false );
	FunctionType *debugft = FunctionType::get( void_type, voidp1, false );
	dao_debug_function = Function::Create( debugft, Function::ExternalLinkage, "dao_debug", llvm_module );
	dao_abs_double = Function::Create( mathft, Function::ExternalLinkage, "abs", llvm_module );
	dao_acos_double = Function::Create( mathft, Function::ExternalLinkage, "acos", llvm_module );
	dao_asin_double = Function::Create( mathft, Function::ExternalLinkage, "asin", llvm_module );
	dao_atan_double = Function::Create( mathft, Function::ExternalLinkage, "atan", llvm_module );
	dao_ceil_double = Function::Create( mathft, Function::ExternalLinkage, "ceil", llvm_module );
	dao_cos_double = Function::Create( mathft, Function::ExternalLinkage, "cos", llvm_module );
	dao_cosh_double = Function::Create( mathft, Function::ExternalLinkage, "cosh", llvm_module );
	dao_exp_double = Function::Create( mathft, Function::ExternalLinkage, "exp", llvm_module );
	dao_floor_double = Function::Create( mathft, Function::ExternalLinkage, "floor", llvm_module );
	dao_log_double = Function::Create( mathft, Function::ExternalLinkage, "log", llvm_module );
	dao_rand_double = Function::Create( mathft, Function::ExternalLinkage, "dao_rand", llvm_module );
	dao_sin_double = Function::Create( mathft, Function::ExternalLinkage, "sin", llvm_module );
	dao_sinh_double = Function::Create( mathft, Function::ExternalLinkage, "sinh", llvm_module );
	dao_sqrt_double = Function::Create( mathft, Function::ExternalLinkage, "sqrt", llvm_module );
	dao_tan_double = Function::Create( mathft, Function::ExternalLinkage, "tan", llvm_module );
	dao_tanh_double = Function::Create( mathft, Function::ExternalLinkage, "tanh", llvm_module );

	llvm_exe_engine = EngineBuilder( llvm_module ).setEngineKind(EngineKind::JIT).create();
#if 0
	llvm_exe_engine->addGlobalMapping( dao_rand_double, (void*) dao_rand );
	llvm_exe_engine->addGlobalMapping( dao_pow_double, (void*) pow );
	llvm_exe_engine->addGlobalMapping( dao_abs_double, (void*) abs );
	llvm_exe_engine->addGlobalMapping( dao_acos_double, (void*) acos );
	llvm_exe_engine->addGlobalMapping( dao_asin_double, (void*) asin );
	llvm_exe_engine->addGlobalMapping( dao_atan_double, (void*) atan );
	llvm_exe_engine->addGlobalMapping( dao_ceil_double, (void*) ceil );
	llvm_exe_engine->addGlobalMapping( dao_cos_double, (void*) cos );
	llvm_exe_engine->addGlobalMapping( dao_cosh_double, (void*) cosh );
	llvm_exe_engine->addGlobalMapping( dao_exp_double, (void*) exp );
	llvm_exe_engine->addGlobalMapping( dao_floor_double, (void*) floor );
	llvm_exe_engine->addGlobalMapping( dao_log_double, (void*) log );
	llvm_exe_engine->addGlobalMapping( dao_sin_double, (void*) sin );
	llvm_exe_engine->addGlobalMapping( dao_sinh_double, (void*) sinh );
	llvm_exe_engine->addGlobalMapping( dao_sqrt_double, (void*) sqrt );
	llvm_exe_engine->addGlobalMapping( dao_tan_double, (void*) tan );
#endif

	llvm_func_optimizer = new FunctionPassManager( llvm_module );
	llvm_func_optimizer->add(new TargetData(*llvm_exe_engine->getTargetData()));
	llvm_func_optimizer->add(createBasicAliasAnalysisPass());
	llvm_func_optimizer->add(createInstructionCombiningPass());
	llvm_func_optimizer->add(createReassociatePass());
	llvm_func_optimizer->add(createGVNPass());
	//llvm_func_optimizer->add(createCFGSimplificationPass());
	llvm_func_optimizer->doInitialization();
}


// Create a function with signature: void (DaoProcess*,DaoRoutine*)
Function* DaoJitHandle::NewFunction( DaoRoutine *routine, int id )
{
	int i;
	char buf[100];
	std::string name = routine->routName->mbs;
	sprintf( buf, "_daojit_%p_%i", routine, id );
	name += buf;

	jitFunction = cast<Function>( llvm_module->getOrInsertFunction( name, dao_jit_function_type ) );
	entryBlock = BasicBlock::Create( *llvm_context, "EntryBlock", jitFunction );
	secondBlock = BasicBlock::Create( *llvm_context, "Second", jitFunction );
	lastBlock = activeBlock = secondBlock;
	SetInsertPoint( entryBlock );

	Argument *jitcdata = jitFunction->arg_begin();
	jitcdata->setName("JitCallData");

	Value *value = CreateConstGEP2_32( jitcdata, 0, 0 ); // jitcdata->localValues: DaoValue*[]**
	localValues = CreateLoad( value ); // jitcdata->localValues: DaoValue*[]*

	//value = CreateConstGEP2_32( ctx, 0, 10 ); // process->regTypes: DaoType*[]**
	//localTypes = CreateLoad( value ); // process->regTypes: DaoType*[]*

	value = CreateConstGEP2_32( jitcdata, 0, 1 ); // jitcdata->localConsts: DaoValue*[]**
	localConsts = CreateLoad( value ); // jitcdata->localConsts: DaoValue*[]*

	directValues.resize( routine->body->regCount );
	stackValues.resize( routine->body->regCount );
	localRefers.resize( routine->body->regCount );
	for(i=0; i<routine->body->regCount; i++){
		directValues[i] = NULL;
		stackValues[i] = NULL;
		localRefers[i] = NULL;
	}

	localValues->setName( "localValues" );
	localConsts->setName( "routConsts" );
#ifdef DEBUG
#endif
	return jitFunction;
}
BasicBlock* DaoJitHandle::NewBlock( int vmc )
{
	char name[ 256 ];
	iplist<BasicBlock> & blist = jitFunction->getBasicBlockList();
	sprintf( name, "block%i", (unsigned int)blist.size() );
	activeBlock = BasicBlock::Create( *llvm_context, name, jitFunction );
	SetInsertPoint( activeBlock );
	lastBlock = activeBlock;
	return activeBlock;
}
BasicBlock* DaoJitHandle::NewBlock( DaoVmCodeX *vmc )
{
	char name[ 256 ];
	iplist<BasicBlock> & blist = jitFunction->getBasicBlockList();
	sprintf( name, "%s%i", DaoVmCode_GetOpcodeName( vmc->code ), (unsigned int)blist.size() );
	activeBlock = BasicBlock::Create( *llvm_context, name, jitFunction );
	SetInsertPoint( activeBlock );
	lastBlock = activeBlock;
	return activeBlock;
}
void DaoJitHandle::SetActiveBlock( BasicBlock *block )
{
	activeBlock = block;
	SetInsertPoint( activeBlock );
}


struct IndexRange
{
	int start;
	int end;
	IndexRange( int s=0, int e=0 ){ start = s; end = e; }

	bool operator<( const IndexRange & other )const{
		return end < other.start;
	}
};


void DaoJIT_Free( DaoRoutine *routine )
{
}
/*
A compilable block is a block of virtual instructions that only branch within the block, 
or just branch to the instruction right after this block.
*/
void DaoJIT_SearchCompilable( DaoRoutine *routine, std::vector<IndexRange> & segments )
{
	std::map<IndexRange,int> ranges;
	std::map<IndexRange,int>::iterator it;
	DaoValue **routConsts = routine->routConsts->items.items.pValue;
	DaoType **types = routine->body->regType->items.pType;
	DaoVmCodeX *vmc, **vmcs = routine->body->annotCodes->items.pVmc;
	int i, j, m, jump, N = routine->body->annotCodes->size;
	bool compilable, last = false;
	size_t k;
	int case_mode = DAO_CASE_UNORDERED;
	for(i=0; i<N; i++){ // find the maximum blocks
		vmc = vmcs[i];
		compilable = dao_opcode_compilable[ vmc->code ];
		if( vmc->code != DVM_CASE ) case_mode = DAO_CASE_UNORDERED;
		// all branching instructions are assumed to be jit compilable for now,
		// so that they can be checked in the next stage:
		switch( vmc->code ){
		case DVM_MATH :
			j = types[vmc->b]->tid;
			m = types[vmc->c]->tid;
			compilable = j and j <= DAO_DOUBLE and m and m <= DAO_DOUBLE;
			break;
		case DVM_DATA : compilable = vmc->a <= DAO_DOUBLE; break;
		case DVM_GETCL : compilable = vmc->a == 0; break;
		default : break;
		}
		if( compilable ){
			if( last ){
				segments.back().end = i;
			}else{
				segments.push_back( IndexRange( i, i ) );
			}
		}
		last = compilable;
		printf( "%3i  %i: ", i, compilable ); DaoVmCodeX_Print( *vmc, NULL );
	}
	for(k=0; k<segments.size(); k++) ranges[segments[k]] = 1;
	for(k=0; k<segments.size(); k++){
		int code, start = segments[k].start;
		int end = segments[k].end;
		bool modified = false;
		for(j=start; j<=end; j++){
			vmc = vmcs[j];
			code = vmc->code;
			jump = vmc->b;
			if( code != DVM_CASE ) case_mode = DAO_CASE_UNORDERED;
			//printf( "checking %3i: ", j ); DaoVmCodeX_Print( *vmc, NULL );
			switch( code ){
			case DVM_GOTO : case DVM_TEST : case DVM_CATCH : 
			case DVM_SWITCH : case DVM_CASE :
			case DVM_TEST_I : case DVM_TEST_F : case DVM_TEST_D :
				compilable = false;
				switch( code ){
				case DVM_GOTO : case DVM_TEST_I : case DVM_TEST_F : case DVM_TEST_D :
					// branchs out of the block
					compilable = vmc->b >= start and vmc->b <= (end+1);
					break;
				case DVM_SWITCH : 
					m = types[vmc->a]->tid;
					if( m == DAO_INTEGER or m == DAO_ENUM ) case_mode = vmcs[j+1]->c; // first case
					compilable = case_mode >= DAO_CASE_INTS;
					if( vmc->b < start or vmc->b > (end+1) ) compilable = false;
					break;
				case DVM_CASE : 
					case_mode = vmc->c;
					compilable = case_mode >= DAO_CASE_INTS;
					m = routConsts[ vmc->a ]->type;
					if( m != DAO_INTEGER and m != DAO_ENUM ) compilable = false;
					if( vmc->b < start or vmc->b >= (end+1) ) compilable = false;
					break;
				case DVM_CATCH :
					jump = vmc->c;
					break;
				}
				if( compilable == false ){
					//printf( "%3i  %i: ", j, compilable ); DaoVmCodeX_Print( *vmc, NULL );
					// break the block:
					segments.push_back( IndexRange( start, j-1 ) );
					segments.push_back( IndexRange( j+1, end ) );
					// check branching into another block:
					it = ranges.find( IndexRange( j, j ) );
					if( it != ranges.end() ) ranges.erase( it );
					ranges[ IndexRange( start, j-1 ) ] = 1;
					ranges[ IndexRange( j+1, end ) ] = 1;
					// check branching inside block:
					if( jump >= start and jump <= end ){
						it = ranges.find( IndexRange( jump, jump ) );
						if( it != ranges.end() ) ranges.erase( it );
						if( jump < j ){
							segments.push_back( IndexRange( start, jump-1 ) );
							segments.push_back( IndexRange( jump+1, j-1 ) );
							ranges[ IndexRange( start, jump-1 ) ] = 1;
							ranges[ IndexRange( jump+1, j-1 ) ] = 1;
						}else if( jump > j ){
							segments.push_back( IndexRange( j+1, jump-1 ) );
							segments.push_back( IndexRange( jump+1, end ) );
							ranges[ IndexRange( j+1, jump-1 ) ] = 1;
							ranges[ IndexRange( jump+1, end ) ] = 1;
						}
					}else{ // branching into another block:
						it = ranges.find( IndexRange( jump, jump ) );
						if( it != ranges.end() ){
							int start2 = it->first.start;
							int end2 = it->first.end;
							ranges.erase( it );
							segments.push_back( IndexRange( start2, jump-1 ) );
							segments.push_back( IndexRange( jump+1, end2 ) );
							ranges[ IndexRange( start2, jump-1 ) ] = 1;
							ranges[ IndexRange( jump+1, end2 ) ] = 1;
						}
					}
					modified = true;
				}
				break;
			}
			if( modified ) break;
		}
	}
	segments.clear();
	for(it=ranges.begin(); it!=ranges.end(); it++){
		if( it->first.start < it->first.end ) segments.push_back( it->first );
	}
	printf( "number of segments: %i\n", (int)segments.size() );
	for(k=0;k<segments.size();k++) printf( "%3li:%5i%5i\n", k, segments[k].start, segments[k].end );
}


Value* DaoJitHandle::GetLocalConstant( int id )
{
	BasicBlock *current = GetInsertBlock();
	SetInsertPoint( entryBlock );
	Value *value = CreateConstGEP2_32( localConsts, 0, id );
	value = CreateLoad( value );
	SetValueName( value, "locst", id );
	SetInsertPoint( current );
	return value;
}
Value* DaoJitHandle::GetUpConstant( int id )
{
	BasicBlock *current = GetInsertBlock();
	Argument *jitcdata = jitFunction->arg_begin();

	SetInsertPoint( entryBlock );
	Value *value = CreateConstGEP2_32( jitcdata, 0, 7 ); // jitcdata->upConsts: DaoValue*[]**
	value = CreateLoad( value ); // jitcdata->upConsts: DaoValue*[]*
	value = CreateConstGEP2_32( value, 0, id );
	value = CreateLoad( value );
	SetInsertPoint( current );
	return value;
}
void DaoJitHandle::SetValueName( Value *value, const char *name, int id )
{
	char buf[100];
	sprintf( buf, "%s%i", name, id );
	value->setName( buf );
}
Value* DaoJitHandle::GetLocalReference( int reg )
{
	BasicBlock *current = GetInsertBlock();
	Value *refer;
	if( localRefers[reg] ) return localRefers[reg];
	SetInsertPoint( entryBlock );
	refer = CreateConstGEP2_32( localValues, 0, reg );
	localRefers[reg] = refer;
	SetValueName( refer, "loref", reg );
	SetInsertPoint( current );
	return refer;
}
Value* DaoJitHandle::GetLocalValue( int reg )
{
	Value *value;
	value = GetLocalReference( reg );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetLocalValueDataPointer( int reg )
{
	Value *value = GetLocalValue( reg );
	return GetValueDataPointer( value );
}
Value* DaoJitHandle::GetLocalNumberValue( int reg, Type *type )
{
	Value *value = GetLocalValue( reg );
	value = GetValueNumberPointer( value, type );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetValueTypePointer( Value *value )
{
	return CreateConstGEP2_32( value, 0, 0 );
}
Value* DaoJitHandle::GetValueDataPointer( Value *value )
{
	return CreateConstGEP2_32( value, 0, 5 );
}
Value* DaoJitHandle::GetValueNumberPointer( Value *value, Type *type )
{
	value = CreatePointerCast( value, type );
	return CreateConstGEP2_32( value, 0, 5 );
}
Value* DaoJitHandle::Dereference( Value *value )
{
	return CreateLoad( value );
}

Value* DaoJitHandle::CastIntegerValuePointer( Value *value )
{
	return CreatePointerCast( value, dao_integer_type_p );
}
Value* DaoJitHandle::CastFloatValuePointer( Value *value )
{
	return CreatePointerCast( value, dao_float_type_p );
}
Value* DaoJitHandle::CastDoubleValuePointer( Value *value )
{
	return CreatePointerCast( value, dao_double_type_p );
}

Value* DaoJitHandle::CastIntegerPointer( Value *value )
{
	PointerType *type = PointerType::getUnqual( daoint_type );
	return CreatePointerCast( value, type );
}
Value* DaoJitHandle::CastFloatPointer( Value *value )
{
	PointerType *type = PointerType::getUnqual( float_type );
	return CreatePointerCast( value, type );
}
Value* DaoJitHandle::CastDoublePointer( Value *value )
{
	PointerType *type = PointerType::getUnqual( double_type );
	return CreatePointerCast( value, type );
}
Value* DaoJitHandle::GetNumberOperand( int reg )
{
	Value *A;
	BasicBlock *current = GetInsertBlock();
	if( directValues[reg] ) return directValues[reg];
	if( stackValues[reg] ) return CreateLoad( stackValues[reg] );

	DaoType *type = routine->body->regType->items.pType[reg];

	SetInsertPoint( entryBlock );
	stackValues[reg] = CreateAlloca( cxx_number_types[type->tid - DAO_INTEGER] );

	A = GetLocalReference( reg );
	Value *B = CreatePointerCast( A, void_type_p );
	A = CreateLoad( A );
	A = CreatePointerCast( A, dao_number_types[type->tid - DAO_INTEGER] );
	B = CreatePointerCast( A, void_type_p );
	A = CreateConstGEP2_32( A, 0, 5 );
	A = CreateLoad( A );
	CreateStore( A, stackValues[reg] );
	SetInsertPoint( current );
	return CreateLoad( stackValues[reg] );
}
Value* DaoJitHandle::GetValueItem( Value *array, Value *index )
{
	std::vector<Value*> ids;
	if( sizeof(void*) == 8 ){
		ids.push_back( getInt64( 0 ) );
	}else{
		ids.push_back( getInt32( 0 ) );
	}
	ids.push_back( index );
	return CreateGEP( array, ids );
}
Value* DaoJitHandle::GetTupleItems( int reg )
{
	Value *value = GetLocalValue( reg );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_tuple_type_p ); // DaoTuple*
	value = CreateConstGEP2_32( value, 0, 8 ); // tuple->items: DaoValue*[]*
	return value;
}
void DaoJitHandle::AddReturnCodeChecking( Value *retcode, int vmc )
{
	BasicBlock *block = activeBlock;
	Constant *zero = ConstantInt::get( daoint_type, 0 );
	Value *cmp = CreateICmpNE( retcode, zero );

	BasicBlock *bltrue = NewBlock( vmc );
	BasicBlock *blfalse = NewBlock( vmc );
	SetInsertPoint( block );
	CreateCondBr( cmp, bltrue, blfalse );
	SetInsertPoint( bltrue );
	CreateRet( retcode );
	activeBlock = blfalse;
	lastBlock = activeBlock;
	SetInsertPoint( activeBlock );
}
Value* DaoJitHandle::AddIndexChecking( Value *index, Value *size, int vmc )
{
	BasicBlock *block = activeBlock;
	size = Dereference( size );
	size = CreateIntCast( size, daoint_type, false );
	Constant *zero = ConstantInt::get( daoint_type, 0 );
	Value *index2 = CreateAdd( index, size );
	Value *cmp = CreateICmpSLT( index, zero );
	index = CreateSelect( cmp, index2, index );
	cmp = CreateOr( CreateICmpSLT( index, zero ), CreateICmpSGE( index, size ) );

	BasicBlock *bltrue = NewBlock( vmc );
	BasicBlock *blfalse = NewBlock( vmc );
	SetInsertPoint( block );
	CreateCondBr( cmp, bltrue, blfalse );
	SetInsertPoint( bltrue );
	CreateRet( ConstantInt::get( int32_type, (DAO_ERROR_INDEX<<16)|vmc ) );
	activeBlock = blfalse;
	lastBlock = activeBlock;
	SetInsertPoint( activeBlock );
	return index;
}
Value* DaoJitHandle::GetListItem( int reg, int index, int vmc )
{
	Constant *zero = ConstantInt::get( daoint_type, 0 );
	Value *value = GetLocalValue( reg );
	Value *id = GetNumberOperand( index );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_list_type_p );
	value = CreateConstGEP2_32( value, 0, 6 ); // list->items: DArray*

	Value *size = CreateConstGEP2_32( value, 0, 1 );
	id = AddIndexChecking( id, size, vmc );

	value = CreateConstGEP2_32( value, 0, 0 ); // list->items.items.pValue: DaoValue*[]**
	value = Dereference( value ); // list->items.items.pValue: DaoValue*[]*
	std::vector<Value*> ids;
	ids.push_back( zero );
	ids.push_back( id );
	value = CreateGEP( value, ids );
	return Dereference( value );
}
Value* DaoJitHandle::GetClassConstant( int reg, int field )
{
#if 0
	Argument *jitcdata = jitFunction->arg_begin();
	Value *value = CreateConstGEP2_32( jitcdata, 0, 4 ); // jitcdata->classConsts: DaoValue*[]**
	value = CreateLoad( value ); // jitcdata->classConsts: DaoValue*[]*

	SetInsertPoint( activeBlock );
	value = CreateConstGEP2_32( localConsts, 0, id );
	return CreateLoad( value );
#endif

	Value *value = GetLocalValue( reg );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_class_type_p );
	value = CreateConstGEP2_32( value, 0, 8 ); // klass->cstData: DArray**;
	value = Dereference( value ); // klass->cstData: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->cstData->items.pValue: DaoValue*[]**
	value = Dereference( value ); // klass->cstData->data: DaoValue*[]*
	value = CreateConstGEP2_32( value, 0, field );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetClassStatic( int reg, int field )
{
	Value *value = GetLocalValue( reg );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_class_type_p );
	value = CreateConstGEP2_32( value, 0, 9 ); // klass->glbData: DArray**;
	value = Dereference( value ); // klass->glbData: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->glbData->items.pValue: DaoValue*[]**
	value = Dereference( value ); // klass->glbData->data: DaoValue*[]*
	value = CreateConstGEP2_32( value, 0, field );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectConstant( int reg, int field )
{
	Value *value = GetLocalValue( reg );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_object_type_p );
	value = CreateConstGEP2_32( value, 0, 8 /*bit fields as one*/ ); // object->defClass: DaoClass**;
	value = Dereference( value ); // object->defClass: DaoClass*;
	value = CreateConstGEP2_32( value, 0, 8 ); // klass->cstData: DArray**;
	value = Dereference( value ); // klass->cstData: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->cstData->items.pValue: DaoValue*[]**
	value = Dereference( value ); // klass->cstData->data: DaoValue*[]*
	value = CreateConstGEP2_32( value, 0, field );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectStatic( int reg, int field )
{
	Value *value = GetLocalValue( reg );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_object_type_p );
	value = CreateConstGEP2_32( value, 0, 8 /*bit fields as one*/ ); // object->defClass: DaoClass**;
	value = Dereference( value ); // object->defClass: DaoClass*;
	value = CreateConstGEP2_32( value, 0, 9 ); // klass->glbData: DArray**;
	value = Dereference( value ); // klass->glbData: DArray*
	value = CreateConstGEP2_32( value, 0, 0 ); // klass->glbData->items.pValue: DaoValue*[]**
	value = Dereference( value ); // klass->glbData->data: DaoValue*[]*
	value = CreateConstGEP2_32( value, 0, field );
	return CreateLoad( value );
}
Value* DaoJitHandle::GetObjectVariable( int reg, int field )
{
	Value *value = GetLocalValue( reg );
	SetInsertPoint( activeBlock );
	value = CreatePointerCast( value, dao_object_type_p );
	value = CreateConstGEP2_32( value, 0, 10 /*bit fields*/ ); // object->objValues: DaoValue*[]**;
	value = Dereference( value ); // object->objValues: DaoValue*[]*;
	value = CreateConstGEP2_32( value, 0, field );
	return CreateLoad( value );
}
int DaoJitHandle::IsDirectValue( int reg )
{
	daoint i, n, dcount = 0;
	DaoCodeNode *node = currentNode;
	if( node->uses->size == 0 ) return 1; // Unused value;
	if( node->uses->size > 1 ) return 0; // Multi-use value;

	node = node->uses->items.pCnode[0];
	for(i=0,n=node->defs->size; i<n; i++) dcount += node->defs->items.pCnode[i]->lvalue == reg;
	if( dcount > 1 ) return 0; // Multi-defintion value;
	return node->index >= start && node->index <= end;
}
void DaoJitHandle::StoreNumber( Value *value, int reg )
{
	//printf( "%3i  %i\n", reg, IsDirectValue( reg ) );
	if( IsDirectValue( reg ) ){
		directValues[reg] = value;
		return;
	}
	BasicBlock *current = GetInsertBlock();
	if( stackValues[reg] == NULL ){
		DaoType *type = routine->body->regType->items.pType[reg];
		SetInsertPoint( entryBlock );
		stackValues[reg] = CreateAlloca( cxx_number_types[type->tid - DAO_INTEGER] );
	}
	SetInsertPoint( current );
	CreateStore( value, stackValues[reg] );
}
Value* DaoJitHandle::MoveValue( Value *dA, Value *dC, Type *type )
{
	dA = GetValueNumberPointer( dA, type );
	dC = GetValueNumberPointer( dC, type );
	dA = Dereference( dA );
	return CreateStore( dA, dC );
}
Function* DaoJitHandle::Compile( int start, int end )
{
	DaoCodeNode *node, **nodes = optimizer->nodes->items.pCnode;
	DaoValue **routConsts = routine->routConsts->items.items.pValue;
	DaoType **types = routine->body->regType->items.pType;
	DaoVmCodeX *vmc, **vmcs = routine->body->annotCodes->items.pVmc;
	Function *jitfunc = NewFunction( routine, start );
	Function *mathfunc = NULL;

	Argument *arg_context = jitFunction->arg_begin();
	Constant *zero32 = ConstantInt::get( int32_type, 0 );
	Constant *zero8 = ConstantInt::get( int8_type, 0 );
	Constant *zero = ConstantInt::get( daoint_type, 0 );
	Constant *tid8_integer = ConstantInt::get( int8_type, DAO_INTEGER );
	Constant *tid8_float = ConstantInt::get( int8_type, DAO_FLOAT );
	Constant *tid8_double = ConstantInt::get( int8_type, DAO_DOUBLE );
	Constant *type_ids[DAO_DOUBLE+1] = { zero8, tid8_integer, tid8_float, tid8_double };
	Value *type, *vdata, *dA, *dB, *dC, *tmp;
	Value *value=NULL, *refer=NULL;
	Type *numtype = NULL;
	ConstantInt *caseint;
	SwitchInst *inswitch;
	int code, i, k, m;

	std::map<Value*,Value*> stores;
	std::map<int,Value*> comparisons;
	std::map<int,BasicBlock*> branchings;
	std::map<int,BasicBlock*> labels;
	std::map<int,BasicBlock*>::iterator iter, stop;

	this->start = start;
	this->end = end;
	firstNode = nodes[start];
	lastNode = nodes[end];
	for(i=start; i<=end; i++){
		vmc = vmcs[i];
		code = vmc->code;
		switch( code ){
		case DVM_GOTO : case DVM_TEST_I : case DVM_TEST_F : case DVM_TEST_D :
			branchings[i] = NULL;
			labels[i+1] = NULL;
			labels[vmc->b] = NULL;
			if( vmc->b > start ) branchings[vmc->b-1] = NULL;
			break;
		case DVM_SWITCH :
			branchings[i] = NULL;
			labels[i+1] = NULL;
			labels[vmc->b] = NULL;
			branchings[vmc->b-1] = NULL;
			// use DVM_CASE to add labels
			for(k=1; k<=vmc->c; k++) labels[vmcs[i+k]->b] = NULL;
			break;
		}
	}

	for(i=start; i<=end; i++){
		Constant *vmcIndex = ConstantInt::get( int32_type, i );
		vmc = vmcs[i];
		code = vmc->code;
		currentNode = nodes[i];
		printf( "%3i ", i ); DaoVmCodeX_Print( *vmc, NULL );
		if( labels.find( i ) != labels.end() ){
			labels[i] = NewBlock( vmc );
			printf( "%3i  %9p  ", i, labels[i] ); DaoVmCodeX_Print( *vmc, NULL );
		}
		switch( code ){
		case DVM_NOP :
			break;
		case DVM_DATA :
			value = getInt32( (int) vmc->b );
			switch( vmc->a ){
			case DAO_NONE : // XXX
				goto Failed;
				break;
			case DAO_INTEGER : break;
			case DAO_FLOAT : value = CreateUIToFP( value, float_type ); break;
			case DAO_DOUBLE : value = CreateUIToFP( value, double_type ); break;
			default: goto Failed;
			}
			if( vmc->a ) StoreNumber( value, vmc->c );
			break;
		case DVM_GETCL :
			if( vmc->a ) goto Failed;
			dC = GetLocalReference( vmc->c );
			dB = GetLocalConstant( vmc->b );
			tmp = CreateCall2( dao_move_pp, dB, dC );
			break;
		case DVM_MATH :
			dB = GetNumberOperand( vmc->b );
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateSIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			switch( vmc->a ){
			case DVM_MATH_ABS  : mathfunc = dao_abs_double;  break;
			case DVM_MATH_ACOS : mathfunc = dao_acos_double; break;
			case DVM_MATH_ASIN : mathfunc = dao_asin_double; break;
			case DVM_MATH_ATAN : mathfunc = dao_atan_double; break;
			case DVM_MATH_CEIL : mathfunc = dao_ceil_double; break;
			case DVM_MATH_COS  : mathfunc = dao_cos_double;  break;
			case DVM_MATH_COSH : mathfunc = dao_cosh_double; break;
			case DVM_MATH_EXP  : mathfunc = dao_exp_double;  break;
			case DVM_MATH_FLOOR : mathfunc = dao_floor_double; break;
			case DVM_MATH_LOG  : mathfunc = dao_log_double;  break;
			case DVM_MATH_RAND : mathfunc = dao_rand_double; break;
			case DVM_MATH_SIN  : mathfunc = dao_sin_double;  break;
			case DVM_MATH_SINH : mathfunc = dao_sinh_double; break;
			case DVM_MATH_SQRT : mathfunc = dao_sqrt_double; break;
			case DVM_MATH_TAN  : mathfunc = dao_tan_double;  break;
			case DVM_MATH_TANH : mathfunc = dao_tanh_double; break;
			default : break;
			}
			dB = CreateCall( mathfunc, dB );
			switch( types[ vmc->c ]->tid ){
			case DAO_INTEGER : dB = CreateFPToSI( dB, daoint_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, float_type ); break;
			case DAO_DOUBLE : break;
			}
			StoreNumber( dB, vmc->c );
			break;
		case DVM_DATA_I :
		case DVM_DATA_F :
		case DVM_DATA_D :
			value = getInt32( (int) vmc->b );
			switch( vmc->code ){
			case DVM_DATA_I : break;
			case DVM_DATA_F : value = CreateUIToFP( value, float_type ); break;
			case DVM_DATA_D : value = CreateUIToFP( value, double_type ); break;
			default: goto Failed;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_GETCL_I :
		case DVM_GETCL_F :
		case DVM_GETCL_D :
			if( vmc->a > 1 ) goto Failed;
			numtype = dao_number_types[ vmc->code - DVM_GETCL_I ];
			dB = NULL;
			if( vmc->a ){
				dB = GetUpConstant( vmc->b );
			}else{
				dB = GetLocalConstant( vmc->b );
			}
			SetInsertPoint( entryBlock );
			dB = GetValueNumberPointer( dB, numtype );
			dB = CreateLoad( dB );
			SetInsertPoint( activeBlock );
			StoreNumber( dB, vmc->c );
			break;
		case DVM_GETCK_I : 
		case DVM_GETCK_F : 
		case DVM_GETCK_D : 
		case DVM_GETCG_I : 
		case DVM_GETCG_F : 
		case DVM_GETCG_D : 

		case DVM_GETVH_I : 
		case DVM_GETVH_F : 
		case DVM_GETVH_D : 
		case DVM_GETVL_I : 
		case DVM_GETVL_F : 
		case DVM_GETVL_D : 
		case DVM_GETVO_I : 
		case DVM_GETVO_F : 
		case DVM_GETVO_D : 
		case DVM_GETVK_I : 
		case DVM_GETVK_F : 
		case DVM_GETVK_D : 
		case DVM_GETVG_I : 
		case DVM_GETVG_F : 
		case DVM_GETVG_D : 

		case DVM_SETVH_II : 
		case DVM_SETVH_IF : 
		case DVM_SETVH_ID : 
		case DVM_SETVH_FI : 
		case DVM_SETVH_FF : 
		case DVM_SETVH_FD : 
		case DVM_SETVH_DI : 
		case DVM_SETVH_DF : 
		case DVM_SETVH_DD : 
		case DVM_SETVL_II : 
		case DVM_SETVL_IF : 
		case DVM_SETVL_ID : 
		case DVM_SETVL_FI : 
		case DVM_SETVL_FF : 
		case DVM_SETVL_FD : 
		case DVM_SETVL_DI : 
		case DVM_SETVL_DF : 
		case DVM_SETVL_DD : 
		case DVM_SETVO_II : 
		case DVM_SETVO_IF : 
		case DVM_SETVO_ID : 
		case DVM_SETVO_FI : 
		case DVM_SETVO_FF : 
		case DVM_SETVO_FD : 
		case DVM_SETVO_DI : 
		case DVM_SETVO_DF : 
		case DVM_SETVO_DD : 
		case DVM_SETVK_II : 
		case DVM_SETVK_IF : 
		case DVM_SETVK_ID : 
		case DVM_SETVK_FI : 
		case DVM_SETVK_FF : 
		case DVM_SETVK_FD : 
		case DVM_SETVK_DI : 
		case DVM_SETVK_DF : 
		case DVM_SETVK_DD : 
		case DVM_SETVG_II : 
		case DVM_SETVG_IF : 
		case DVM_SETVG_ID : 
		case DVM_SETVG_FI : 
		case DVM_SETVG_FF : 
		case DVM_SETVG_FD : 
		case DVM_SETVG_DI : 
		case DVM_SETVG_DF : 
		case DVM_SETVG_DD : 
			goto Failed;
			break;
		case DVM_MOVE_II : case DVM_MOVE_IF : case DVM_MOVE_ID :
		case DVM_MOVE_FI : case DVM_MOVE_FF : case DVM_MOVE_FD :
		case DVM_MOVE_DI : case DVM_MOVE_DF : case DVM_MOVE_DD :
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = GetNumberOperand( vmc->a ); break;
			case DAO_FLOAT   : dA = GetNumberOperand( vmc->a ); break;
			case DAO_DOUBLE  : dA = GetNumberOperand( vmc->a ); break;
			}
			switch( code ){
			case DVM_MOVE_IF : 
			case DVM_MOVE_ID : dA = CreateFPToSI( dA, daoint_type ); break;
			case DVM_MOVE_FI : dA = CreateSIToFP( dA, float_type ); break;
			case DVM_MOVE_DI : dA = CreateSIToFP( dA, double_type ); break;
			case DVM_MOVE_FD : dA = CreateFPCast( dA, float_type ); break;
			case DVM_MOVE_DF : dA = CreateFPCast( dA, double_type ); break;
			}
			StoreNumber( dA, vmc->c );
			break;
		case DVM_ADD_III :
		case DVM_SUB_III :
		case DVM_MUL_III :
		case DVM_DIV_III :
		case DVM_MOD_III :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_ADD_III : value = CreateAdd( dA, dB ); break;
			case DVM_SUB_III : value = CreateSub( dA, dB ); break;
			case DVM_MUL_III : value = CreateMul( dA, dB ); break;
			case DVM_DIV_III : value = CreateSDiv( dA, dB ); break;
			case DVM_MOD_III : value = CreateSRem( dA, dB ); break;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_III :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dA = CreateSIToFP( dA, double_type );
			dB = CreateSIToFP( dB, double_type );
			tmp = CreateCall2( dao_pow_double, dA, dB );
			tmp = CreateFPToSI( tmp, daoint_type );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_AND_III :
		case DVM_OR_III :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_AND_III : value = CreateICmpEQ( dA, zero ); break;
			case DVM_OR_III  : value = CreateICmpNE( dA, zero ); break;
			}
			value = CreateSelect( value, dA, dB );
			StoreNumber( value, vmc->c );
			break;
		case DVM_LT_III :
		case DVM_LE_III :
		case DVM_EQ_III :
		case DVM_NE_III :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_LT_III : value = CreateICmpSLT( dA, dB ); break;
			case DVM_LE_III : value = CreateICmpSLE( dA, dB ); break;
			case DVM_EQ_III : value = CreateICmpEQ( dA, dB ); break;
			case DVM_NE_III : value = CreateICmpNE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_I and m <= DVM_TEST_D) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						break;
					}
				}
			}
			value = CreateIntCast( value, daoint_type, false );
			StoreNumber( value, vmc->c );
			break;
		case DVM_BITAND_III :
		case DVM_BITOR_III :
		case DVM_BITXOR_III :
		case DVM_BITLFT_III :
		case DVM_BITRIT_III :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_BITAND_III : value = CreateAnd( dA, dB ); break;
			case DVM_BITOR_III : value = CreateOr( dA, dB ); break;
			case DVM_BITXOR_III : value = CreateXor( dA, dB ); break;
			case DVM_BITLFT_III : value = CreateShl( dA, dB ); break;
			case DVM_BITRIT_III : value = CreateLShr( dA, dB ); break;
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_FFF :
		case DVM_SUB_FFF :
		case DVM_MUL_FFF :
		case DVM_DIV_FFF :
		case DVM_MOD_FFF :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_ADD_FFF : value = CreateFAdd( dA, dB ); break;
			case DVM_SUB_FFF : value = CreateFSub( dA, dB ); break;
			case DVM_MUL_FFF : value = CreateFMul( dA, dB ); break;
			case DVM_DIV_FFF : value = CreateFDiv( dA, dB ); break;
			case DVM_MOD_FFF : value = CreateFRem( dA, dB ); break;
							   // XXX: float mod in daovm.
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_FFF :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dA = CreateFPCast( dA, double_type );
			dB = CreateFPCast( dB, double_type );
			tmp = CreateCall2( dao_pow_double, dA, dB );
			tmp = CreateFPCast( tmp, float_type );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_AND_FFF :
		case DVM_OR_FFF :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			tmp = CreateUIToFP( zero, float_type );
			switch( code ){
			case DVM_AND_FFF : value = CreateFCmpOEQ( dA, tmp ); break;
			case DVM_OR_FFF  : value = CreateFCmpONE( dA, tmp ); break;
			}
			value = CreateSelect( value, dA, dB );
			StoreNumber( value, vmc->c );
			break;
		case DVM_LT_FFF :
		case DVM_LE_FFF :
		case DVM_EQ_FFF :
		case DVM_NE_FFF :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_LT_FFF : value = CreateFCmpOLT( dA, dB ); break;
			case DVM_LE_FFF : value = CreateFCmpOLE( dA, dB ); break;
			case DVM_EQ_FFF : value = CreateFCmpOEQ( dA, dB ); break;
			case DVM_NE_FFF : value = CreateFCmpONE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_I and m <= DVM_TEST_D) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						break;
					}
				}
			}
			value = CreateUIToFP( value, float_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_BITAND_FFF :
		case DVM_BITOR_FFF :
		case DVM_BITXOR_FFF :
		case DVM_BITLFT_FFF :
		case DVM_BITRIT_FFF :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dA = CreateFPToUI( dA, daoint_type );
			dB = CreateFPToUI( dB, daoint_type );
			switch( code ){
			case DVM_BITAND_FFF : value = CreateAnd( dA, dB ); break;
			case DVM_BITOR_FFF : value = CreateOr( dA, dB ); break;
			case DVM_BITXOR_FFF : value = CreateXor( dA, dB ); break;
			case DVM_BITLFT_FFF : value = CreateShl( dA, dB ); break;
			case DVM_BITRIT_FFF : value = CreateLShr( dA, dB ); break;
			}
			value = CreateUIToFP( value, float_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_DDD :
		case DVM_SUB_DDD :
		case DVM_MUL_DDD :
		case DVM_DIV_DDD :
		case DVM_MOD_DDD :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_ADD_DDD : value = CreateFAdd( dA, dB ); break;
			case DVM_SUB_DDD : value = CreateFSub( dA, dB ); break;
			case DVM_MUL_DDD : value = CreateFMul( dA, dB ); break;
			case DVM_DIV_DDD : value = CreateFDiv( dA, dB ); break;
			case DVM_MOD_DDD : value = CreateFRem( dA, dB ); break;
							   // XXX: float mod in daovm.
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_DDD :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			tmp = CreateCall2( dao_pow_double, dA, dB );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_AND_DDD :
		case DVM_OR_DDD :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			tmp = CreateUIToFP( zero, double_type );
			switch( code ){
			case DVM_AND_DDD : value = CreateFCmpOEQ( dA, tmp ); break;
			case DVM_OR_DDD  : value = CreateFCmpONE( dA, tmp ); break;
			}
			value = CreateSelect( value, dA, dB );
			StoreNumber( value, vmc->c );
			break;
		case DVM_LT_DDD :
		case DVM_LE_DDD :
		case DVM_EQ_DDD :
		case DVM_NE_DDD :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( code ){
			case DVM_LT_DDD : value = CreateFCmpOLT( dA, dB ); break;
			case DVM_LE_DDD : value = CreateFCmpOLE( dA, dB ); break;
			case DVM_EQ_DDD : value = CreateFCmpOEQ( dA, dB ); break;
			case DVM_NE_DDD : value = CreateFCmpONE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_I and m <= DVM_TEST_D) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						break;
					}
				}
			}
			value = CreateUIToFP( value, double_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_BITAND_DDD :
		case DVM_BITOR_DDD :
		case DVM_BITXOR_DDD :
		case DVM_BITLFT_DDD :
		case DVM_BITRIT_DDD :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dA = CreateFPToUI( dA, int32_type );
			dB = CreateFPToUI( dB, int32_type );
			switch( code ){
			case DVM_BITAND_DDD : value = CreateAnd( dA, dB ); break;
			case DVM_BITOR_DDD : value = CreateOr( dA, dB ); break;
			case DVM_BITXOR_DDD : value = CreateXor( dA, dB ); break;
			case DVM_BITLFT_DDD : value = CreateShl( dA, dB ); break;
			case DVM_BITRIT_DDD : value = CreateLShr( dA, dB ); break;
			}
			value = CreateUIToFP( value, double_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_FNN :
		case DVM_SUB_FNN :
		case DVM_MUL_FNN :
		case DVM_DIV_FNN :
		case DVM_MOD_FNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = CreateUIToFP( dA, double_type ); break;
			case DAO_FLOAT : dA = CreateFPCast( dA, double_type ); break;
			}
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateUIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			switch( code ){
			case DVM_ADD_FNN : value = CreateFAdd( dA, dB ); break;
			case DVM_SUB_FNN : value = CreateFSub( dA, dB ); break;
			case DVM_MUL_FNN : value = CreateFMul( dA, dB ); break;
			case DVM_DIV_FNN : value = CreateFDiv( dA, dB ); break;
			case DVM_MOD_FNN : value = CreateFRem( dA, dB ); break;
							   // XXX: float mod in daovm.
			}
			value = CreateFPCast( value, float_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_FNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dA = CreateFPCast( dA, double_type );
			dB = CreateFPCast( dB, double_type );
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = CreateUIToFP( dA, double_type ); break;
			case DAO_FLOAT : dA = CreateFPCast( dA, double_type ); break;
			}
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateUIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			tmp = CreateCall2( dao_pow_double, dA, dB );
			tmp = CreateFPCast( tmp, float_type );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_LT_FNN :
		case DVM_LE_FNN :
		case DVM_EQ_FNN :
		case DVM_NE_FNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = CreateUIToFP( dA, double_type ); break;
			case DAO_FLOAT : dA = CreateFPCast( dA, double_type ); break;
			}
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateUIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			switch( code ){
			case DVM_LT_FNN : value = CreateFCmpOLT( dA, dB ); break;
			case DVM_LE_FNN : value = CreateFCmpOLE( dA, dB ); break;
			case DVM_EQ_FNN : value = CreateFCmpOEQ( dA, dB ); break;
			case DVM_NE_FNN : value = CreateFCmpONE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_I and m <= DVM_TEST_D) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						break;
					}
				}
			}
			value = CreateUIToFP( value, float_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_BITLFT_FNN :
		case DVM_BITRIT_FNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			if( types[ vmc->a ]->tid != DAO_INTEGER ) dA = CreateFPToUI( dA, daoint_type );
			if( types[ vmc->b ]->tid != DAO_INTEGER ) dB = CreateFPToUI( dB, daoint_type );
			switch( code ){
			case DVM_BITLFT_FNN : value = CreateShl( dA, dB ); break;
			case DVM_BITRIT_FNN : value = CreateLShr( dA, dB ); break;
			}
			value = CreateUIToFP( value, float_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_DNN :
		case DVM_SUB_DNN :
		case DVM_MUL_DNN :
		case DVM_DIV_DNN :
		case DVM_MOD_DNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = CreateUIToFP( dA, double_type ); break;
			case DAO_FLOAT : dA = CreateFPCast( dA, double_type ); break;
			}
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateUIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			switch( code ){
			case DVM_ADD_DNN : value = CreateFAdd( dA, dB ); break;
			case DVM_SUB_DNN : value = CreateFSub( dA, dB ); break;
			case DVM_MUL_DNN : value = CreateFMul( dA, dB ); break;
			case DVM_DIV_DNN : value = CreateFDiv( dA, dB ); break;
			case DVM_MOD_DNN : value = CreateFRem( dA, dB ); break;
							   // XXX: float mod in daovm.
			}
			StoreNumber( value, vmc->c );
			break;
		case DVM_POW_DNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			dA = CreateFPCast( dA, double_type );
			dB = CreateFPCast( dB, double_type );
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = CreateUIToFP( dA, double_type ); break;
			case DAO_FLOAT : dA = CreateFPCast( dA, double_type ); break;
			}
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateUIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			tmp = CreateCall2( dao_pow_double, dA, dB );
			StoreNumber( tmp, vmc->c );
			break;
		case DVM_LT_DNN :
		case DVM_LE_DNN :
		case DVM_EQ_DNN :
		case DVM_NE_DNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			switch( types[ vmc->a ]->tid ){
			case DAO_INTEGER : dA = CreateUIToFP( dA, double_type ); break;
			case DAO_FLOAT : dA = CreateFPCast( dA, double_type ); break;
			}
			switch( types[ vmc->b ]->tid ){
			case DAO_INTEGER : dB = CreateUIToFP( dB, double_type ); break;
			case DAO_FLOAT : dB = CreateFPCast( dB, double_type ); break;
			}
			switch( code ){
			case DVM_LT_DNN : value = CreateFCmpOLT( dA, dB ); break;
			case DVM_LE_DNN : value = CreateFCmpOLE( dA, dB ); break;
			case DVM_EQ_DNN : value = CreateFCmpOEQ( dA, dB ); break;
			case DVM_NE_DNN : value = CreateFCmpONE( dA, dB ); break;
			}
			if( i < end ){
				m = vmcs[i+1]->code;
				if( m == DVM_TEST or (m >= DVM_TEST_I and m <= DVM_TEST_D) ){
					if( vmc->c == vmcs[i+1]->a ){
						comparisons[ i+1 ] = value;
						break;
					}
				}
			}
			value = CreateUIToFP( value, double_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_BITLFT_DNN :
		case DVM_BITRIT_DNN :
			dA = GetNumberOperand( vmc->a );
			dB = GetNumberOperand( vmc->b );
			if( types[ vmc->a ]->tid != DAO_INTEGER ) dA = CreateFPToUI( dA, daoint_type );
			if( types[ vmc->b ]->tid != DAO_INTEGER ) dB = CreateFPToUI( dB, daoint_type );
			switch( code ){
			case DVM_BITLFT_DNN : value = CreateShl( dA, dB ); break;
			case DVM_BITRIT_DNN : value = CreateLShr( dA, dB ); break;
			}
			value = CreateUIToFP( value, double_type );
			StoreNumber( value, vmc->c );
			break;
		case DVM_ADD_SS :
			dA = GetLocalValue( vmc->a );
			dB = GetLocalValue( vmc->b );
			dC = GetLocalReference( vmc->c );
			CreateCall3( dao_string_add, dA, dB, dC );
			break;
		case DVM_LT_SS :
		case DVM_LE_SS :
		case DVM_EQ_SS :
		case DVM_NE_SS :
			dA = GetLocalValue( vmc->a );
			dB = GetLocalValue( vmc->b );
			dC = GetLocalValue( vmc->c );
			switch( code ){
			case DVM_LT_SS : CreateCall3( dao_string_lt, dA, dB, dC ); break;
			case DVM_LE_SS : CreateCall3( dao_string_le, dA, dB, dC ); break;
			case DVM_EQ_SS : CreateCall3( dao_string_eq, dA, dB, dC ); break;
			case DVM_NE_SS : CreateCall3( dao_string_ne, dA, dB, dC ); break;
			}
			break;
		case DVM_GOTO : case DVM_TEST_I : case DVM_TEST_F : case DVM_TEST_D : 
		case DVM_SWITCH : case DVM_CASE :
			break;
		case DVM_MOVE_SS :
			dA = GetLocalValue( vmc->a );
			dC = GetLocalReference( vmc->c );
			CreateCall2( dao_string_move, dA, dC );
			break;
		case DVM_MOVE_PP : 
			value = GetLocalValue( vmc->a );
			refer = GetLocalReference( vmc->c );
			CreateCall2( dao_move_pp, value, refer );
			break;
		case DVM_GETI_SI :
			dA = GetLocalValue( vmc->a );
			dB = GetLocalValue( vmc->b );
			dC = GetLocalValue( vmc->c );
			tmp = CreateCall4( dao_geti_si, dA, dB, dC, vmcIndex );
			AddReturnCodeChecking( tmp, i );
			break;
		case DVM_SETI_SII :
			dA = GetLocalValue( vmc->a );
			dB = GetLocalValue( vmc->b );
			dC = GetLocalValue( vmc->c );
			tmp = CreateCall4( dao_seti_sii, dA, dB, dC, vmcIndex );
			AddReturnCodeChecking( tmp, i );
			break;
		case DVM_GETI_LI : 
			value = GetListItem( vmc->a, vmc->b, i );
			refer = GetLocalReference( vmc->c );
			CreateCall2( dao_move_pp, value, refer );
			//if( modes[vmc->c] == DAO_REG_REFER ){
			//	tmp = CreateStore( value, refer );
			//}else{
			//refer = Dereference( refer );
			//CreateCall2( dao_value_copy, refer, value );
			//}
			break;
		case DVM_SETI_LI : 
			dA = GetLocalValue( vmc->a );
			dB = GetLocalValue( vmc->b );
			dC = GetLocalValue( vmc->c );
			tmp = CreateCall4( dao_seti_li, dA, dB, dC, vmcIndex );
			AddReturnCodeChecking( tmp, i );
			break;
		case DVM_GETI_LII : case DVM_GETI_LFI : case DVM_GETI_LDI :
			value = GetListItem( vmc->a, vmc->b, i );
			refer = GetLocalReference( vmc->c );
			tmp = CreateCall2( dao_move_pp, value, refer );
			//if( modes[vmc->c] == DAO_REG_REFER ){
			//	tmp = CreateStore( value, refer );
			//}else{
			//value = GetValueDataPointer( value );
			//value = Dereference( value );
			//refer = Dereference( refer );
			//refer = GetValueDataPointer( refer );
			//}
			break;
		case DVM_GETI_LSI :
			dA = GetListItem( vmc->a, vmc->b, i );
			dC = GetLocalReference( vmc->c );
			tmp = CreateCall2( dao_move_pp, dA, dC );
			//CreateCall2( dao_string_move, dA, dC );
			break;
		case DVM_SETI_LIII :
		case DVM_SETI_LIIF :
		case DVM_SETI_LIID :
		case DVM_SETI_LFII :
		case DVM_SETI_LFIF :
		case DVM_SETI_LFID :
		case DVM_SETI_LDII :
		case DVM_SETI_LDIF :
		case DVM_SETI_LDID :
			dC = GetListItem( vmc->c, vmc->b, i );
			switch( types[vmc->a]->tid ){
			case DAO_INTEGER : dA = GetNumberOperand( vmc->a ); break;
			case DAO_FLOAT  : dA = GetNumberOperand( vmc->a ); break;
			default : dA = GetNumberOperand( vmc->a ); break;
			}
			switch( code ){
			case DVM_SETI_LIII :
			case DVM_SETI_LIIF :
			case DVM_SETI_LIID :
				if( code != DVM_SETI_LIII ) dA = CreateFPToSI( dA, daoint_type );
				dC = CastIntegerValuePointer( dC );
				break;
			case DVM_SETI_LFII :
			case DVM_SETI_LFIF :
			case DVM_SETI_LFID :
				switch( code ){
				case DVM_SETI_LFII : dA = CreateSIToFP( dA, float_type ); break;
				case DVM_SETI_LFID : dA = CreateFPCast( dA, float_type ); break;
				}
				dC = CastFloatValuePointer( dC );
				break;
			case DVM_SETI_LDII :
			case DVM_SETI_LDIF :
			case DVM_SETI_LDID :
				switch( code ){
				case DVM_SETI_LFII : dA = CreateSIToFP( dA, double_type ); break;
				case DVM_SETI_LFID : dA = CreateFPCast( dA, double_type ); break;
				}
				dC = CastDoubleValuePointer( dC );
				break;
			}
			dC = GetValueDataPointer( dC );
			tmp = CreateStore( dA, dC );
			break;
		case DVM_GETI_TI :
			value = GetTupleItems( vmc->a );
			dB = GetNumberOperand( vmc->b );
			value = GetValueItem( value, dB );
			refer = GetLocalReference( vmc->c );
			tmp = CreateCall2( dao_move_pp, value, refer );
			break;
		case DVM_GETF_TX :
			value = GetTupleItems( vmc->a );
			value = CreateConstGEP2_32( value, 0, vmc->b );
			value = Dereference( value );
			refer = GetLocalReference( vmc->c );
			tmp = CreateCall2( dao_move_pp, value, refer );
			break;
		case DVM_GETF_TI :
		case DVM_GETF_TF :
		case DVM_GETF_TD :
			dA = GetTupleItems( vmc->a );
			dA = CreateConstGEP2_32( dA, 0, vmc->b );
			dA = Dereference( dA );
			dC = GetLocalValue( vmc->c );
			numtype = dao_number_types[ code - DVM_GETF_TI ];
			dA = GetValueNumberPointer( dA, numtype );
			dC = GetValueNumberPointer( dC, numtype );
			CreateStore( CreateLoad( dA ), dC );
			break;
		case DVM_SETI_TI : 
			dA = GetLocalValue( vmc->a );
			dB = GetLocalValue( vmc->b );
			dC = GetLocalValue( vmc->c );
			tmp = CreateCall4( dao_seti_ti, dA, dB, dC, vmcIndex );
			AddReturnCodeChecking( tmp, i );
			break;
		case DVM_SETF_TPP : 
			dA = GetLocalValue( vmc->a );
			dB = ConstantInt::get( int32_type, vmc->b );
			dC = GetLocalValue( vmc->c );
			tmp = CreateCall4( dao_setf_t, dA, dB, dC, vmcIndex );
			AddReturnCodeChecking( tmp, i );
			break;
		case DVM_SETF_TII :
		case DVM_SETF_TIF :
		case DVM_SETF_TID :
		case DVM_SETF_TFI :
		case DVM_SETF_TFF :
		case DVM_SETF_TFD :
		case DVM_SETF_TDI :
		case DVM_SETF_TDF :
		case DVM_SETF_TDD :
			dC = GetTupleItems( vmc->c );
			dC = CreateConstGEP2_32( dC, 0, vmc->b );
			dC = Dereference( dC );
			dA = NULL;
			switch( types[vmc->a]->tid ){
			case DAO_INTEGER : dA = GetNumberOperand( vmc->a ); break;
			case DAO_FLOAT  : dA = GetNumberOperand( vmc->a ); break;
			default : dA = GetNumberOperand( vmc->a ); break;
			}
			switch( code ){
			case DVM_SETF_TII :
				dC = CastIntegerValuePointer( dC );
				break;
			case DVM_SETF_TIF :
				dC = CastIntegerValuePointer( dC );
				dA = CreateFPToSI( dA, daoint_type );
				break;
			case DVM_SETF_TID :
				dC = CastIntegerValuePointer( dC );
				dA = CreateFPToSI( dA, daoint_type );
				break;
			case DVM_SETF_TFI :
				dC = CastFloatValuePointer( dC );
				dA = CreateSIToFP( dA, float_type );
				break;
			case DVM_SETF_TFF :
				dC = CastFloatValuePointer( dC );
				break;
			case DVM_SETF_TFD :
				dC = CastFloatValuePointer( dC );
				dA = CreateFPCast( dA, float_type );
				break;
			case DVM_SETF_TDI :
				dC = CastDoubleValuePointer( dC );
				dA = CreateSIToFP( dA, double_type );
				break;
			case DVM_SETF_TDF :
				dC = CastDoubleValuePointer( dC );
				dA = CreateFPCast( dA, float_type );
				break;
			case DVM_SETF_TDD :
				dC = CastDoubleValuePointer( dC );
				break;
			}
			dC = GetValueDataPointer( dC );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_TSS :
			dA = GetLocalValue( vmc->a );
			dC = GetTupleItems( vmc->c );
			dC = CreateConstGEP2_32( dC, 0, vmc->b );
			CreateCall2( dao_string_move, dA, dC );
			break;
		case DVM_GETF_KCI :
		case DVM_GETF_KCF :
		case DVM_GETF_KCD :
			dA = GetClassConstant( vmc->a, vmc->b );
			dC = GetLocalValue( vmc->c );
			MoveValue( dA, dC, dao_number_types[ code - DVM_GETF_KCI ] );
			break;
		case DVM_GETF_KGI :
		case DVM_GETF_KGF :
		case DVM_GETF_KGD :
			dA = GetClassStatic( vmc->a, vmc->b );
			dC = GetLocalValue( vmc->c );
			MoveValue( dA, dC, dao_number_types[ code - DVM_GETF_KGI ] );
			break;
		case DVM_GETF_OCI :
		case DVM_GETF_OCF :
		case DVM_GETF_OCD :
			dA = GetObjectConstant( vmc->a, vmc->b );
			dC = GetLocalValue( vmc->c );
			MoveValue( dA, dC, dao_number_types[ code - DVM_GETF_OCI ] );
			break;
		case DVM_GETF_OGI :
		case DVM_GETF_OGF :
		case DVM_GETF_OGD :
			dA = GetObjectStatic( vmc->a, vmc->b );
			dC = GetLocalValue( vmc->c );
			MoveValue( dA, dC, dao_number_types[ code - DVM_GETF_OGI ] );
			break;
		case DVM_GETF_OVI :
		case DVM_GETF_OVF :
		case DVM_GETF_OVD :
			dA = GetObjectVariable( vmc->a, vmc->b );
			dC = GetLocalValue( vmc->c );
			MoveValue( dA, dC, dao_number_types[ code - DVM_GETF_OVI ] );
			break;
		case DVM_SETF_KGII :
		case DVM_SETF_KGIF :
		case DVM_SETF_KGID :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_KGII ] );
			dC = GetClassStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_integer_type );
			if( code != DVM_SETF_KGII ) dA = CreateFPToSI( dA, daoint_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_KGFI :
		case DVM_SETF_KGFF :
		case DVM_SETF_KGFD :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_KGFI ] );
			dC = GetClassStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_float_type );
			if( code == DVM_SETF_KGFI ) dA = CreateSIToFP( dA, float_type );
			else if( code == DVM_SETF_KGFD ) dA = CreateFPCast( dA, float_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_KGDI :
		case DVM_SETF_KGDF :
		case DVM_SETF_KGDD :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_KGDI ] );
			dC = GetClassStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_double_type );
			if( code == DVM_SETF_KGDI ) dA = CreateSIToFP( dA, double_type );
			else if( code == DVM_SETF_KGDF ) dA = CreateFPCast( dA, double_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OGII :
		case DVM_SETF_OGIF :
		case DVM_SETF_OGID :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_OGII ] );
			dC = GetObjectStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_integer_type );
			if( code != DVM_SETF_KGII ) dA = CreateFPToSI( dA, daoint_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OGFI :
		case DVM_SETF_OGFF :
		case DVM_SETF_OGFD :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_OGFI ] );
			dC = GetObjectStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_float_type );
			if( code == DVM_SETF_KGFI ) dA = CreateSIToFP( dA, float_type );
			else if( code == DVM_SETF_KGFD ) dA = CreateFPCast( dA, float_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OGDI :
		case DVM_SETF_OGDF :
		case DVM_SETF_OGDD :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_OGDI ] );
			dC = GetObjectStatic( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_double_type );
			if( code == DVM_SETF_KGDI ) dA = CreateSIToFP( dA, double_type );
			else if( code == DVM_SETF_KGDF ) dA = CreateFPCast( dA, double_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OVII :
		case DVM_SETF_OVIF :
		case DVM_SETF_OVID :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_OVII ] );
			dC = GetObjectVariable( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_integer_type );
			if( code != DVM_SETF_KGII ) dA = CreateFPToSI( dA, daoint_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OVFI :
		case DVM_SETF_OVFF :
		case DVM_SETF_OVFD :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_OVFI ] );
			dC = GetObjectVariable( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_float_type );
			if( code == DVM_SETF_KGFI ) dA = CreateSIToFP( dA, float_type );
			else if( code == DVM_SETF_KGFD ) dA = CreateFPCast( dA, float_type );
			CreateStore( dA, dC );
			break;
		case DVM_SETF_OVDI :
		case DVM_SETF_OVDF :
		case DVM_SETF_OVDD :
			dA = GetLocalNumberValue( vmc->a, dao_number_types[ code - DVM_SETF_OVDI ] );
			dC = GetObjectVariable( vmc->c, vmc->b );
			dC = GetValueNumberPointer( dC, dao_double_type );
			if( code == DVM_SETF_KGDI ) dA = CreateSIToFP( dA, double_type );
			else if( code == DVM_SETF_KGDF ) dA = CreateFPCast( dA, double_type );
			CreateStore( dA, dC );
			break;
		default : goto Failed;
		}
		if( branchings.find( i ) != branchings.end() ) branchings[i] = activeBlock;
		//if( code == DVM_SWITCH ) i += vmc->c + 1; // skip cases and one goto.
	}
	if( labels.find( start ) != labels.end() ){
		SetInsertPoint( entryBlock );
		CreateBr( secondBlock );
	}
	for(iter=branchings.begin(), stop=branchings.end(); iter!=stop; iter++){
		vmc = vmcs[ iter->first ];
		code = vmc->code;
		k = iter->first + 1;
		switch( code ){
		case DVM_GOTO : case DVM_TEST_I : case DVM_TEST_F : case DVM_TEST_D :
		case DVM_SWITCH :
			k = vmc->b;
			break;
		}
		if( k > end ){
			labels[end+1] = NewBlock( vmcs[end+1] );
			break;
		}
	}
	for(iter=labels.begin(), stop=labels.end(); iter!=stop; iter++){
		printf( "%3i %9p\n", iter->first, iter->second );
	}
	for(iter=branchings.begin(), stop=branchings.end(); iter!=stop; iter++){
		vmc = vmcs[ iter->first ];
		code = vmc->code;
		if( labels[ vmc->b ] == NULL ) labels[vmc->b] = lastBlock;
		//printf( "%3i %3i %p\n", iter->first, vmc->b, labels[vmc->b] );
		printf( "%3i %9p: ", iter->first, iter->second ); DaoVmCodeX_Print( *vmc, NULL );
		SetActiveBlock( iter->second );
		switch( code ){
		case DVM_GOTO :
			CreateBr( labels[ vmc->b ] );
			break;
		case DVM_TEST_I :
			value = NULL;
			if( comparisons.find( iter->first ) != comparisons.end() ){
				value = comparisons[iter->first];
			}else{
				dA = GetNumberOperand( vmc->a );
				value = CreateICmpNE( dA, zero );
			}
			CreateCondBr( value, labels[ iter->first + 1 ], labels[ vmc->b ] );
			break;
		case DVM_TEST_F :
			value = NULL;
			if( comparisons.find( iter->first ) != comparisons.end() ){
				value = comparisons[iter->first];
			}else{
				dA = GetNumberOperand( vmc->a );
				value = CreateUIToFP( zero, float_type );
				value = CreateFCmpONE( dA, value );
			}
			CreateCondBr( value, labels[ iter->first + 1 ], labels[ vmc->b ] );
			break;
		case DVM_TEST_D :
			value = NULL;
			if( comparisons.find( iter->first ) != comparisons.end() ){
				value = comparisons[iter->first];
			}else{
				dA = GetNumberOperand( vmc->a );
				value = CreateUIToFP( zero, double_type );
				value = CreateFCmpONE( dA, value );
			}
			CreateCondBr( value, labels[ iter->first + 1 ], labels[ vmc->b ] );
			break;
		case DVM_SWITCH :
			m = types[vmc->a]->tid; // integer or enum
			if( m == DAO_ENUM ){
				dA = GetLocalValue( vmc->a );
				dA = CreatePointerCast( dA, dao_enum_type_p );
				dA = CreateConstGEP2_32( dA, 0, 6 );
				dA = Dereference( dA );
			}else{
				dA = GetNumberOperand( vmc->a );
			}
			inswitch = CreateSwitch( dA, labels[ vmc->b ], vmc->c );
			// use DVM_CASE to add switch labels
			for(k=1; k<=vmc->c; k++){
				DaoVmCodeX *vmc2 = vmcs[ iter->first + k ];
				daoint ic = routConsts[vmc2->a]->xInteger.value;
				if( m == DAO_ENUM ) ic = routConsts[vmc2->a]->xEnum.value;
				caseint = cast<ConstantInt>( ConstantInt::get( daoint_type, ic ) );
				inswitch->addCase( caseint, labels[vmc2->b] );
			}
			break;
		default :
			if( iter->first < end ){
				CreateBr( labels[ iter->first + 1 ] );
			}else{
				CreateBr( labels[ end + 1 ] );
			}
			break;
		}
	}
	printf( "lastBlock = %p\n", lastBlock ) ;
	SetActiveBlock( lastBlock );
	node = optimizer->nodes->items.pCnode[end];
	for(i=0; i<routine->body->regCount; i++){
		DaoType *type = types[i];
		printf( "%3i:  %p  %p\n", i, directValues[i], stackValues[i] );
		if( stackValues[i] == NULL ) continue;
		if( GET_BIT( node->bits->mbs, i ) == 0 ) continue; // skip dead variables;
		printf( "%3i:  %p  %p\n", i, directValues[i], stackValues[i] );
		Value *A = CreateLoad( stackValues[i] );
		Value *C = GetLocalValue( i );
		C = GetValueNumberPointer( C, dao_number_types[type->tid - DAO_INTEGER] );
		CreateStore( A, C );
	}
	CreateRet( zero32 );
	SetInsertPoint( entryBlock );
	if( entryBlock->back().isTerminator() == false ) CreateBr( secondBlock );
	if( secondBlock->size() ==0 or secondBlock->back().isTerminator() == false ){
		Function::BasicBlockListType & blocks = jitfunc->getBasicBlockList();
		SetInsertPoint( secondBlock );
		if( blocks.size() > 2 )
			CreateBr( ++(++blocks.begin()) );
		else
			CreateRet( zero32 );
	}
#if 0
	if( 1 ){
		BasicBlock::iterator iter;
		for(iter=entryBlock->begin(); iter!=entryBlock->end(); iter++){
			iter->print( outs() );
			printf( "\n" );
		}
	}
#endif
	return jitfunc;
Failed:
	printf( "failed compiling: %s %4i %4i\n", routine->routName->mbs, start, end );
	jitfunc->eraseFromParent();
	return NULL;
}
void DaoJIT_Compile( DaoRoutine *routine, DaoOptimizer *optimizer )
{
	printf( "routine %s\n", routine->routName->mbs );
	DaoJitHandle handle( *llvm_context, routine, optimizer );
	std::vector<Function*> jitFunctions;
	std::vector<IndexRange> segments;
	DaoJIT_SearchCompilable( routine, segments );
	if( segments.size() == 0 ) return;
	DaoOptimizer_LinkDU( optimizer, routine );
	DaoOptimizer_DoLVA( optimizer, routine );
	for(int i=0, n=segments.size(); i<n; i++){
		if( (segments[i].end - segments[i].start) < 10 ) continue;
		//if( (segments[i].end - segments[i].start) < 3 ) continue;
		printf( "compiling: %5i %5i\n", segments[i].start, segments[i].end );
		Function *jitfunc = handle.Compile( segments[i].start, segments[i].end );
		printf( "compiled: %p\n", jitfunc );
		if( jitfunc == NULL ) continue;
		llvm_func_optimizer->run( *jitfunc );

		DaoVmCode *vmc = routine->body->vmCodes->codes + segments[i].start;
		vmc->code = DVM_JITC;
		vmc->a = jitFunctions.size();
		vmc->b = segments[i].end - segments[i].start + 1;
		jitFunctions.push_back( jitfunc );
	}
	if( jitFunctions.size() ) routine->body->jitData = new std::vector<Function*>( jitFunctions );
	//return;
	PassManager PM;
	PM.add(createPrintModulePass(&outs()));
	PM.run(*llvm_module);
	verifyModule(*llvm_module, PrintMessageAction);
}

typedef int (*DaoJitFunction)( DaoJitCallData *data );

void DaoJIT_Execute( DaoProcess *process, DaoJitCallData *data, int jitcode )
{
	DaoRoutine *routine = process->activeRoutine;
	std::vector<Function*> & jitFuncs = *(std::vector<Function*>*) routine->body->jitData;

#if 0
	std::vector<GenericValue> Args(2);
	Args[0].PointerVal = process;
	Args[1].PointerVal = routine;
	GenericValue GV = llvm_exe_engine->runFunction( jitFuncs[jitcode], Args);
#endif

	void *fptr = llvm_exe_engine->getPointerToFunction( jitFuncs[jitcode] );
	DaoJitFunction jitfunc = (DaoJitFunction) fptr;
	int rc = (*jitfunc)( data );
	if( rc ){
		int vmc = rc & 0xffff;
		int ec = rc >> 16;
		process->activeCode = process->topFrame->codes + vmc;
		DaoProcess_RaiseException( process, ec, "" );
	}
}

