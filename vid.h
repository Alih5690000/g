#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL_image.h>
#include "vec.c"
typedef struct Video{
    Vector* frames;
    size_t pos;
    SDL_Renderer* renderer;
    float fps;
    float* dt;
    float acc;
} Video;

Video* Video_create(const char* name, SDL_Renderer* r,int fps,float* dt){
    Video* v=(Video*)malloc(sizeof(Video));
    v->renderer=r;
    v->fps=fps;
    v->acc=0.f;
    v->frames=CreateVector(sizeof(SDL_Texture*));
    Vector_Resize(v->frames,1024);
    v->dt=dt;
    int count=1;
    while (true){
            SDL_Surface* surf;
            string* str=CreateString();
            String_addc(str,name);
            String_addc(str,"/frame");
            char tmp[20];
            snprintf(tmp,20,"%d",count);
            String_addc(str,tmp);
            String_addc(str,".png");
            surf=IMG_Load(String_cstr(str));
            if (!surf){
                break;
            }
            SDL_Texture* t=SDL_CreateTextureFromSurface(r,surf);
            if (!t)
                break;
            Vector_PushBack(v->frames,&t);
            String_Free(str);
            SDL_FreeSurface(surf);
            count++;
        }
    Vector_Shrink(v->frames);
    v->pos=0;
    emscripten_log(EM_LOG_CONSOLE,"Loaded %d frames for video %s",count-1,name);   
    return v;
}

SDL_Texture* Video_getFrame(Video* v){
    SDL_Texture* res=*((SDL_Texture**)Vector_Get(v->frames,v->pos));
    return res;
}

void Video_destroy(Video* v){
    Vector_Free(v->frames);
    free(v);
}

void Video_update(Video* v){
    v->acc+=*v->dt;
    while(v->acc>=1.f/(v->fps)){
        v->acc-=1.f/(v->fps);
        v->pos+=1;
    }
    if (v->pos>=Vector_Size(v->frames)){
        v->pos=0;
    }
}

void Video_setPos(Video* v,size_t pos){
    if (pos>=Vector_Size(v->frames)){
        const char* mes="Out of range: Video setPos";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return;
    }
    v->pos=pos;
}
