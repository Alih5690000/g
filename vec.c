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
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define VECTOR_FOR(obj,name,type) \
    for(type* name=(type*)__Vector_begin(obj);\
        name<(type*)__Vector_begin(obj)+Vector_Size(obj);\
            name+=1)

#define BINARY 1<<1
#define RP 1<<2
#define APPEND 1<<3

typedef char*(*StrFunc)(void*);
typedef void*(*AddFunc)(const void*,const void*);
typedef void*(*SubFunc)(const void*,const void*);
typedef void*(*MulFunc)(const void*,const void*);
typedef void*(*DivFunc)(const void*,const void*);
typedef void*(*CopyFunc)(const void*);

typedef struct BasicObj{
    StrFunc str;
    AddFunc add;
    MulFunc mul;
    DivFunc di;
    SubFunc sub;
    CopyFunc copy;
} BasicObj;

typedef struct Var{
    BasicObj* var;
} Var;

void Var_set(Var* obj,BasicObj* o){
    obj->var=o;
}

char* Var_str(Var* o){
    return o->var->str(o->var);
}

static char __errbuf[1024];

const char* GetError(){
    return __errbuf;
}

typedef struct array{
    void* __data;
    int __ellsize;
    int __size;
} array;

array* CreateArray(int size,int ellsize){
    array* obj=(array*)malloc(sizeof(array));
    obj->__data=malloc(size*ellsize);
    obj->__ellsize=ellsize;
    obj->__size=size;
    return obj;
}

void* __Array_get(array* o,int index){
    if (index>o->__size){
        const char* mes="Out of range: array";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return NULL;
    }
    return ((char*)(o->__data))+(o->__ellsize*index);
}

void Array_Free(array* o){
    free(o->__data);
    free(o);
}

int Array_size(array* obj){
    return obj->__size;
}

array* Array_copy(array* o){
    array* n=CreateArray(o->__size,o->__ellsize);
    memcpy(n->__data,o->__data,o->__size*o->__ellsize);
    return n;
}

typedef struct Vector{
    void* __data;
    int __size;
    int __pos;
    int __ellsize;
} Vector;

#define __VECTOR_DEFAULT_CAPACITY 5
#define __STRING_DEFAULT_CAPACITY 10

void* __Vector_begin(Vector* v){
    return v->__data;
}

void* __Vector_end(Vector* v){
    return ((char*)(v->__data))+(v->__pos*v->__ellsize);
}

Vector* CreateVector(int ellement_size){
    Vector* v=(Vector*)malloc(sizeof(Vector));
    v->__ellsize=ellement_size;
    v->__pos=0;
    v->__size=__VECTOR_DEFAULT_CAPACITY;
    v->__data=malloc(v->__size*v->__ellsize);
    return v;
}

void Vector_Resize(Vector* v, int newsize){
    void* n = malloc(v->__ellsize * newsize);
    int count = v->__pos;
    if (count > newsize) count = newsize;
    memcpy(n, v->__data, count * v->__ellsize);
    free(v->__data);
    v->__data = n;
    v->__size = newsize;
    if (v->__pos > newsize) v->__pos = newsize;
}

void Vector_Shrink(Vector* v){
    Vector_Resize(v,v->__pos);
}

void Vector_erase(Vector* v,int pos){
    if (pos>=v->__pos || pos<0){
        const char* mes="Invalid pos to erease: Vector";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return;
    }
   memmove(
    (char*)v->__data + v->__ellsize * pos,
    (char*)v->__data + v->__ellsize * (pos + 1),
    (v->__pos - pos - 1) * v->__ellsize
    );
    v->__pos--;
}

void Vector_insert(Vector* v, int pos, const void* data){
    if (v->__pos >= v->__size){
        Vector_Resize(v, v->__size * 2);
    }
    memmove(
        (char*)v->__data + v->__ellsize * (pos + 1),
        (char*)v->__data + v->__ellsize * pos,
        (v->__pos - pos) * v->__ellsize
    );
    memcpy(
        (char*)v->__data + v->__ellsize * pos,
        data,
        v->__ellsize
    );
    v->__pos++;
}


void Vector_PushBack(Vector* v,const void* ell){
    char* ptr=(char*)v->__data;
    if (v->__pos>=v->__size){
        printf("resized");
        fflush(stdout);
        Vector_Resize(v,v->__size*2);
    }
    memcpy(ptr+v->__pos*v->__ellsize,ell,v->__ellsize);
    v->__pos+=1;
}

void Vector_PopBack(Vector* v){
    if (v->__pos==0){
        const char* mes="Already in the end cannot Pop back: Vector";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return;
    }
    v->__pos-=1;
}

int Vector_Size(Vector* v){
    return v->__pos;
}

int Vector_Capacity(Vector* v){
    return v->__size;
}

int __Vector_ellsize(Vector* v){
    return v->__ellsize;
}

void* Vector_Get(Vector* v,int index){
    if (index>=v->__pos || index<0){
        const char* mes="Out of range: Vector";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return NULL;
    }
    char* ptr=(char*)v->__data;
    void* a=(void*)(ptr+index*v->__ellsize);
    return a;
}

void Vector_clear(Vector* o){
    o->__pos=0;
}

Vector* Vector_Copy(Vector* v){
    Vector* n=CreateVector(v->__ellsize);
    Vector_Resize(n,v->__size);
    memcpy(n->__data,v->__data,v->__pos*v->__ellsize);
    return n;
}

void Vector_Free(Vector *v){
    free(v->__data);
    free(v);
}

typedef struct string{
    char* __data;
    int __size;
    int __curr;
} string;

string* CreateString(){
    string* obj=(string*)malloc(sizeof(string));
    obj->__data=(char*)malloc(__STRING_DEFAULT_CAPACITY);
    obj->__size=__STRING_DEFAULT_CAPACITY;
    obj->__curr=0;
    obj->__data[obj->__curr]='\0';
    return obj;
}

const char* String_cstr(string* obj){
    obj->__data[obj->__curr]='\0';
    return obj->__data;
}

void String_Free(string* obj){
    free(obj->__data);
    free(obj);
}

void __String_resize(string* s,int size){
    char* data=(char*)malloc(size);
    memcpy(data,s->__data,s->__curr);
    free(s->__data);
    s->__data=data;
    s->__size=size;
}

string* String_addc(string* obj,const char* s){
    if (obj->__curr+strlen(s)+1>obj->__size)
        __String_resize(obj,obj->__size+(strlen(s)*2));
    memcpy(obj->__data+obj->__curr,s,strlen(s));
    obj->__curr+=strlen(s);
    obj->__data[obj->__curr] = '\0';
    return obj;
}

string* String_adds(string* obj,string* other){
    if (obj->__curr+other->__curr+1>obj->__size)
        __String_resize(obj,other->__size*2);
    memcpy(obj->__data+obj->__curr,other->__data,other->__curr);
    obj->__curr+=other->__curr;
    obj->__data[obj->__curr] = '\0';
    return obj;
}

string* String_Copy(string* o){
    string* n=CreateString();
    String_adds(n,o);
    return n;
}

int String_size(string* o){
    return o->__curr;
}

void String_set(string* o,const char* s){
    memcpy(o->__data,s,strlen(s));
    o->__curr=strlen(s);
}

string* CreateStringWithStr(const char* str){
    string* res=CreateString();
    String_addc(res,str);
    return res;
}

typedef struct file{
    FILE* f;
} file;

typedef struct Ifile{
    file f;
} Ifile;

Ifile* CreateIfile(const char* name,int flags){
    Ifile* n=(Ifile*)malloc(sizeof(Ifile));
    string* mode=CreateString();
    String_addc(mode,"r");
    if (flags & BINARY)
        String_addc(mode,"b");
    if (flags & RP)
        String_addc(mode,"+");
    n->f.f=fopen(name,String_cstr(mode));
    if (!n->f.f){
        string* err=CreateString();
        String_addc(err,strerror(errno));
        String_addc(err,": Ifile");
        memcpy(__errbuf,String_cstr(err),String_size(err));
        String_Free(err);
        return NULL;
    }
    String_Free(mode);
    return n;
}

void Ifile_read(Ifile* file,int bytes,void* dst){
    fread(dst,1,bytes,file->f.f);
}

void IFile_close(Ifile* f){
    fclose(f->f.f);
    free(f->f.f);
}

typedef struct Ofile{
    file f;
} Ofile;

void OFile_close(Ofile* f){
    fclose(f->f.f);
    free(f->f.f);
}

Ofile* CreateOfile(const char* name,int flags){
    Ofile* n=(Ofile*)malloc(sizeof(Ofile));
    string* mode=CreateString();
    if ((flags & APPEND)!=0)
        String_addc(mode,"a");
    else    
        String_addc(mode,"w");
    if (flags & BINARY){
        String_addc(mode,"b");
    }
    if (flags & RP){
        String_addc(mode,"+");
    }
    n->f.f=fopen(name,String_cstr(mode));
    if (!n->f.f){
        string* err=CreateString();
        String_addc(err,strerror(errno));
        String_addc(err,": Ofile");
        memcpy(__errbuf,String_cstr(err),String_size(err));
        String_Free(err);
        return NULL;
    }
    String_Free(mode);
    return n;
}

void Ofile_write(Ofile* file,int bytes,const void* data){
    fwrite(data,1,bytes,file->f.f);
}

typedef struct Thread{
    pthread_t thread;
} Thread;

Thread* CreateThread(){
    Thread* t=malloc(sizeof(Thread));
    return t;
}

void Thread_start(Thread* o,void* (*func)(void*),void* args){
    pthread_create(
        &o->thread,
        NULL,
        func,
        args
    );
}

void* Thread_join(Thread* o){
    void* res;
    pthread_join(o->thread,&res);
    return res;
}

void Thread_detach(Thread* o){
    pthread_detach(o->thread);
}

typedef struct Mutex{
    pthread_mutex_t mutex;
} Mutex;

Mutex* Mutex_create(){
    Mutex* res=malloc(sizeof(Mutex));
    pthread_mutex_init(&res->mutex,NULL);
    return res;
}

void Mutex_lock(Mutex* o){
    pthread_mutex_lock(&o->mutex);
}

void Mutex_unlock(Mutex* o){
    pthread_mutex_unlock(&o->mutex);
}

typedef struct ConditionVariable{
    pthread_cond_t cv;
} ConditionVariable;

ConditionVariable* CreateConditionVariable(){
    ConditionVariable* res=malloc(sizeof(ConditionVariable));
    pthread_cond_init(
        &res->cv,
        NULL
    );
    return res;
}

void ConditonVariable_waitCond(ConditionVariable* o,int (*func)(),Mutex* m){
    while (!func()){
        pthread_cond_wait(
            &o->cv,
            &m->mutex
        );
    }
}

void ConditionVariable_wait(ConditionVariable* o,Mutex* m){
    pthread_cond_wait(&o->cv,&m->mutex);
}

void ConditionVariable_notify(ConditionVariable* o){
    pthread_cond_signal(&o->cv);
}
#endif