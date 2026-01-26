/*
MIT License

Copyright (c) 2025 Kaldbay Alihan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef VEC_C
#define VEC_C
#include <stdlib.h>

#define new(type,count) (type*)malloc(sizeof(type)*count)

#define ForcelyCast(ForcelyCast_from,ForcelyCast_to,ForcelyCast_obj,ForcelyCast_result) { \
    ForcelyCast_to* ForcelyCast_res=(ForcelyCast_to*)malloc(sizeof(ForcelyCast_to));\
    memcpy(ForcelyCast_res,(ForcelyCast_to*)&ForcelyCast_obj,sizeof(ForcelyCast_from));\
    ForcelyCast_to ForcelyCast_ret=*ForcelyCast_res;\
    free(ForcelyCast_res); \
    ForcelyCast_result=ForcelyCast_ret; \
}

/**
 * \brief Dynamic storage of data (unknown type)
 */
typedef struct Vector Vector;

/**
 * \brief makes deep copy of Vector
 * \param v the source
 * \return a new identical Vector object
 */
Vector* Vector_Copy(Vector* v);

/**
 *  \brief Creates Vector where ellement size is first argument
 *  \param ellement_size Size of one ellement
 *  \return returnes a Vector object with malloced data
 */
Vector* CreateVector(int __ellement_size);
/**
 * \brief Changes size of Vector
 * \param v Vector pointer
 * \param newsize The new size of Vector
 */

/*
*/
void Vector_erease(Vector* v,int pos);

/*
*/
void Vector_insert(Vector* v,int pos,const void* data);

void Vector_Resize(Vector* _v,int __newsize);
/**
 * \brief Adds element to the end of Vector
 * \param v Vector pointer
 * \param ell pointer to the object
 * \note Do not worry ellement_size data from ell will be copyed
 */
void Vector_PushBack(Vector* _v,const void* __ell);
/**
 * \brief Its seems obvious
 * \param v Vector pointer
 * \return Returns how many ellements of element_size are alredy in Vector
 */
int Vector_Size(Vector* _v);
/**
 * \brief This one returns the size of memory that reserved to Vector
 * \param v You know
 * \return how many elements under the memory reserved
 */
int Vector_Capacity(Vector* _v);
/**
 * \brief Due to C is not OOP language that the alternate of operator[]
 * \param v Use your brain
 * \param index Just an index
 * \note Returns NULL in case of out-of-range access
 */
void* Vector_Get(Vector* _v,int __index);
/**
 * \brief Free memory
 * \param v Kinda obvious
 * \note After you free Vector can not be used
 */

#define VECTOR_GET(obj,index,type) *(type*)Vector_Get(obj,(index))

void Vector_Free(Vector* _v);
/**
 * \brief deletes the last element of vector
 * \param v Dont ask me
 */
void Vector_PopBack(Vector* _v);

void* __Vector_begin(Vector* _v);

void* __Vector_end(Vector* _v);

int __Vector_ellsize(Vector* v);

#define VECTOR_FOR(obj,name,type) \
    for(type* name=(type*)__Vector_begin(obj);\
        name<(type*)__Vector_end(obj);\
            name+=1)


/**
 * \brief dynamic storage of text
 */
typedef struct string string;

/**
 * \brief creates a string object
 * \return a pointer to object
 */
string* CreateString();

/**
 * \brief copies string
 * \param o source
 * \return a identical string object
 */
string* String_Copy(string* o);

/**
 * \brief turn string object to a const char*
 * \param obj the string object itself
 * \return the C string
 */
const char* String_cstr(string* __obj);

/**
 * \brief Frees the memory locked by string
 * \param obj string object
 */
void String_Free(string* __obj);

/**
 * \brief adds string object with const char*
 * \param obj the string object
 * \param s the C string
 */
string* String_addc(string* __obj,const char* _s);

/**
 * \brief adds two string objects together
 * \param obj Dst object
 * \param other Src object
 */
string* String_adds(string* obj,string* other);

/**
 * \brief Returns size of string
 * \param o object
 * \return size of o
 */
int String_size(string* o);

/**
 * \brief sets to a C string
 * \param o you know
 * \param s C string to set
 */
void String_set(string* o,const char* s);

string* CreateStringWithStr(const char* str);

/**
 * \brief array but better
 */
typedef struct array array;
/**
 * \brief creates array
 * \param size size of array that cant be changed
 * \param ellsize size of element
 * \return array object
 */
array* CreateArray(int size,int ellsize);

/**
 * \brief get a pointer to a element from array
 * \param o array object
 * \param index index
 * \return a adress of element
 */
void* __Array_get(array* __o,int _index);

#define Array_Get(obj,i,type) *((type*)__Array_get(obj,i))

/**
 * \brief destroy array object
 * \param o the object
 */
void Array_Free(array* __o);

/**
 * \brief get the size of array
 * \param obj a object
 * \return size of array
 */
int Array_size(array* __obj);

/**
 * \brief copy array object
 * \param o source
 * \return new identical array object
 */
array* Array_copy(array* o);

const char* GetError();

#define BINARY 1<<1
#define RP 1<<2
#define APPEND 1<<3

/**
 * \brief a cool version of FILE*
 */
typedef struct file file;

void File_close(void* file);

/**
 * \brief a output file object
 */
typedef struct Ofile Ofile;

/**
 * \brief creates Ofile object
 * \param name file name
 * \param flags flags
 */
Ofile* CreateOfile(const char* name,int flags);

/**
 * \brief writes to a Ofile object
 * \param file the file object
 * \param bytes the size of data
 * \param data adress
 */
void Ofile_write(Ofile* file,int bytes,const void* data);

/**
 * \brief a input file object
 */
typedef struct Ifile Ifile;

/**
 * \brief create Ifile object
 * \param name a file name
 * \param flags flags
 */
Ifile* CreateIfile(const char* name,int flags);

/**
 * \brief read from Ifile object
 * \param file the Ifile object
 * \param bytes the size of data
 * \param a adress of data
 */
void Ifile_read(Ifile* file,int bytes,void* dst);

typedef struct Flag{
    string* name;
    int state;
} Flag;

void Flag_free(Flag* o);

typedef struct State{
    Vector* flags;
} State;

State CreateState();

void State_free(State* o);

int State_isOn(State* o,const char* name);

void State_set(State* o,const char* name,int to);

/**
 * \brief a wrap around pthread_t
 */
typedef struct Thread Thread;

/**
 * \brief Create a Thread object
 */
Thread* CreateThread();

/**
 * \brief launch a Thread
 * \param o a Object
 * \param func a function to call
 * \param args a pointer at parameters to call function with
 */
void Thread_start(Thread* o,void* (*func)(void*),void* args);

/**
 * \brief detaches a Thread
 * \param o a Thread object
 */
void Thread_detach(Thread* o);

/**
 * \brief a Mutex, wrap around pthread_mutex_t
 */
typedef struct Mutex Mutex;

/**
 * \brief Creates a Mutex
 */
Mutex* Mutex_create();

/**
 * \brief a locks access to other threads to data
 * \param o object
 */
void Mutex_lock(Mutex* o);

/**
 * \brief unlocks a mutex lock
 * \param o object
 */
void Mutex_unlock(Mutex* o);



#endif



