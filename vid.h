#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL_image.h>
#include <vec.c>
typedef struct Video{
    Vector* frames;
    size_t pos;
    SDL_Renderer* renderer;
} Video;

Video* Video_create(const char* name, SDL_Renderer* r){
    Video* v=(Video*)malloc(sizeof(Video));
    v->renderer=r;
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
    v->pos=0;
    return v;
}

SDL_Texture* Video_getFrame(Video* v){
    if (v->pos>=Vector_Size(v->frames)){
        v->pos=0;
    }
    SDL_Texture* res=*((SDL_Texture**)Vector_Get(v->frames,v->pos));
    v->pos++;
    return res;
}

void Video_destroy(Video* v){
    Vector_Free(v->frames);
    free(v);
}

void Video_setPos(Video* v,size_t pos){
    if (pos>=Vector_Size(v->frames)){
        const char* mes="Out of range: Video setPos";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return;
    }
    v->pos=pos;
}
