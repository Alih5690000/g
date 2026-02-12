#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL_image.h>
#include <emscripten/html5.h>
#include "vec.c"

SDL_Texture* DeepCopyTexture(SDL_Renderer* renderer, SDL_Texture* src)
{
    int w, h;
    Uint32 format;
    int access;

    if (SDL_QueryTexture(src, &format, &access, &w, &h) != 0)
        return NULL;

    SDL_Texture* dst = SDL_CreateTexture(
        renderer,
        format,
        SDL_TEXTUREACCESS_TARGET,
        w,
        h
    );

    if (!dst)
        return NULL;

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);

    SDL_SetRenderTarget(renderer, dst);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, src, NULL, NULL);

    SDL_SetRenderTarget(renderer, oldTarget);

    return dst;
}

SDL_Texture* DeepCopyTextureEx(SDL_Renderer* renderer, SDL_Texture* src,
    SDL_RendererFlip flip, double angle, SDL_Point* center)
{
    int w, h;
    Uint32 format;
    int access;

    if (SDL_QueryTexture(src, &format, &access, &w, &h) != 0)
        return NULL;

    SDL_Texture* dst = SDL_CreateTexture(
        renderer,
        format,
        SDL_TEXTUREACCESS_TARGET,
        w,
        h
    );

    SDL_SetTextureBlendMode(dst, SDL_BLENDMODE_BLEND);

    if (!dst)
        return NULL;

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);

    SDL_SetRenderTarget(renderer, dst);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopyEx(renderer, src, NULL, NULL, angle, center, flip);

    SDL_SetRenderTarget(renderer, oldTarget);

    return dst;
}

typedef struct Video{
    Vector* frames;
    size_t pos;
    SDL_Renderer* renderer;
    float fps;
    float* dt;
    int loops;
    float acc;
} Video;

Video* Video_create(const char* name, SDL_Renderer* r,int fps,float* dt){
    Video* v=(Video*)malloc(sizeof(Video));
    v->renderer=r;
    v->fps=fps;
    v->acc=0.f;
    v->loops=0;
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

Video* Video_CreateDull(){
    Video* v=(Video*)malloc(sizeof(Video));
    v->renderer=NULL;
    v->fps=0.f;
    v->acc=0.f;
    v->loops=0;
    v->frames=CreateVector(sizeof(SDL_Texture*));
    Vector_Resize(v->frames,1024);
    v->dt=NULL;
    return v;
}

SDL_Texture* Video_getFrame(Video* v){
    SDL_Texture* res=*((SDL_Texture**)Vector_Get(v->frames,v->pos));
    return res;
}

void Video_setFps(Video* v,float fps){
    v->fps=fps;
}

SDL_Texture* Video_getFrameEx(Video*o, size_t index){
    if (index>=Vector_Size(o->frames)){
        const char* mes="Out of range: Video getFrameEx";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return NULL;
    }
    SDL_Texture* res=*((SDL_Texture**)Vector_Get(o->frames,index));
    return res;
}

void Video_destroy(Video* v){
    Vector_Free(v->frames);
    free(v);
}

void Video_update(Video* v){
    if (!v) return;
    v->acc+=*v->dt;
    while(v->acc>=1.f/(v->fps)){
        v->acc-=1.f/(v->fps);
        v->pos+=1;
    }
    if (v->pos>=Vector_Size(v->frames)){
        v->loops++;
        v->pos=0;
    }
}

int Video_getLoops(Video* v){
    return v->loops;
}

Video* Video_CopyShallow(Video* o){
    Video* res=malloc(sizeof(Video));
    res->renderer=o->renderer;
    res->fps=o->fps;
    res->acc=0.f;
    res->pos = 0;
    res->loops = 0;
    res->dt=o->dt;
    res->frames=CreateVector(sizeof(SDL_Texture*));
    Vector_Resize(res->frames,Vector_Size(o->frames));
    for (int i=0;i<Vector_Size(o->frames);i++){
        SDL_Texture* t=*(SDL_Texture**)Vector_Get(o->frames,i);
        Vector_PushBack(res->frames,&t);
    }
    return res;
}



Video* Video_CopyDeep(Video* o){
    Video* res=malloc(sizeof(Video));
    res->renderer=o->renderer;
    res->fps=o->fps;
    res->acc=o->acc;
    res->pos = o->pos;
    res->loops = o->loops;
    res->dt=o->dt;
    res->frames=CreateVector(sizeof(SDL_Texture*));
    Vector_Resize(res->frames,Vector_Size(o->frames));
    for (int i=0;i<Vector_Size(o->frames);i++){
        SDL_Texture* t=DeepCopyTexture(o->renderer,*(SDL_Texture**)Vector_Get(o->frames,i));
        Vector_PushBack(res->frames,&t);
    }
    return res;
}

size_t Video_getPos(Video* v){
    return v->pos;
}



void Video_setPos(Video* v,size_t pos){
    if (pos>=Vector_Size(v->frames)){
        const char* mes="Out of range: Video setPos";
        memcpy(__errbuf,mes,strlen(mes)+1);
        return;
    }
    v->pos=pos;
}
