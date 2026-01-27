#include <emscripten/html5.h>
#include <SDL.h>
#include "vec.c"

SDL_Texture* Wtexture;
SDL_Renderer* renderer;
SDL_Window* window;
float dt;
int start,end;

void quit(){
    emscripten_cancel_main_loop();
}

typedef struct Sprite{
    SDL_FRect rect;
    SDL_Texture* txt;
    void (*update)(void*);
    void (*destroy)(void*);
} Sprite;

typedef struct Enemy{
    Sprite base;
} Enemy;

typedef struct Weapon{
    Sprite* owner;
    void(*update)(void*);
    void(*onFire)(void*,Vector*);
    void(*asItem)(void*,SDL_FPoint*);
    void(*destroy)(void*);
} Weapon;

typedef struct Sword{
    Weapon base;
    float angle;
    int animOn;
    float cd;
    SDL_Texture* txt;
} Sword;

void Sword_destroy(void* obj){
    Sword* o=(Sword*)obj;
    SDL_DestroyTexture(o->txt);
    free(o);
}

//ens vector of Sprites
void Sword_onFire(void* obj,Vector* ens){
    Sword* o=(Sword*)obj;
    SDL_FRect ownerRect=o->base.owner->rect;
    SDL_FRect dmgRect={ownerRect.x-25,ownerRect.y,ownerRect.w+50,ownerRect.h};
    int j=0;
    VECTOR_FOR(ens,i,Sprite){
        if (SDL_HasIntersectionF(&dmgRect,&i->rect)) {
            i->destroy(i);
            Vector_erase(ens,j);
        }
        j++;
    }
    o->animOn=1;
    o->angle=360.f;
}

void Sword_update(void* obj){
    Sword* o=(Sword*)obj;
    SDL_FRect ownerRect=o->base.owner->rect;
    SDL_FRect drawRect={ownerRect.x+ownerRect.w-75.f,ownerRect.y+(ownerRect.h/2.f),
        10.f,75.f};
    if (o->animOn){
        o->angle-=180*dt;
        if (o->angle<0.f){
            o->angle=0;
            o->animOn=0;
        }
    }
    SDL_RenderCopyExF(renderer,o->txt,NULL,&drawRect,o->angle,&(SDL_FPoint){drawRect.w,drawRect.h},
        SDL_FLIP_NONE);
}

void Sword_asItem(void* obj,SDL_FPoint* point){
    Sword* o=(Sword*)obj;
    SDL_FRect rect={point->x,point->y,10,75};
    SDL_RenderCopyF(renderer,o->txt,NULL,&rect);
}

void* Sword_create(Sprite* owner){
    Sword* res=malloc(sizeof(Sword));
    res->angle=0.f;
    res->animOn=0;
    res->txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STATIC,75,10);
    res->cd=0.f;
    res->base.owner=owner;
    res->base.asItem=Sword_asItem;
    res->base.destroy=Sword_destroy;
    res->base.onFire=Sword_onFire;
    res->base.update=Sword_update;
    return res;
}

void HandleDelta(){
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
}


SDL_FRect plr={0.f,0.f,50.f,50.f};
float plr_dy=0.f;
float plr_dx=0.f;
int plr_inAir=1;
float plr_speed=300.f;
float gravity=900.f;
float plr_dshSpeed=100.f;
int plr_canDash=1;
Weapon* plr_wep;

Vector* walls;
Vector* sprites;

void loop(){

    HandleDelta();

    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            quit();
    }

    const Uint8* keys=SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]){
        if(!plr_inAir){
            plr_dy=500.f;
            plr_inAir=1;
        }
    }

    plr_dx=0.f;

    if (keys[SDL_SCANCODE_A]){
        plr_dx=plr_speed*dt*-1;
    }
    if (keys[SDL_SCANCODE_D]){
        plr_dx=plr_speed*dt;
    }
    if (keys[SDL_SCANCODE_E] && plr_canDash){
        plr_canDash=0;
        plr_dx=plr_dshSpeed*(plr_dx<0 ? -1 : 1);
    }
    if (!keys[SDL_SCANCODE_E]){
        plr_canDash=1;
    }
    Uint32 mstate=SDL_GetMouseState(NULL,NULL);
    if (mstate & SDL_BUTTON_LMASK){
        plr_wep->onFire((void*)plr_wep,sprites);
    }
    plr_wep->update((void*)plr_wep);

    plr.x+=plr_dx;

    VECTOR_FOR(walls,i,SDL_FRect){
        if (SDL_HasIntersectionF(&plr,i)){
            if (plr_dx>0)
                plr.x=i->x-plr.w;
            else
                plr.x=i->x+i->w;
        }
    }

    plr_inAir=1;

    if (plr_inAir){
        plr_dy-=gravity*dt;
    }
    plr.y-=plr_dy*dt;

    VECTOR_FOR(walls,i,SDL_FRect){
        if (SDL_HasIntersectionF(&plr,i)){
            if (plr.y<=i->y){
                plr.y=i->y-plr.h;
                plr_inAir=0;
            }
            else{
                plr.y=i->y+i->h;
            }
            plr_dy=0;

        }
    }

    SDL_SetRenderTarget(renderer,Wtexture);

    {
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,0,0,255,255);
        SDL_RenderFillRectF(renderer,&plr);
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        VECTOR_FOR(walls,i,SDL_FRect){
            SDL_RenderFillRectF(renderer,i);
        }
        VECTOR_FOR(sprites,i,Sprite){
            i->update((void*)i);
        }
    }

    SDL_SetRenderTarget(renderer,NULL);
    SDL_RenderCopy(renderer,Wtexture,NULL,NULL);

    SDL_RenderPresent(renderer);
}

int main(){
    walls=CreateVector(sizeof(SDL_FRect));
    sprites=CreateVector(sizeof(Sprite*));
    {
        Vector_PushBack(walls,&(SDL_FRect){0.f,700.f,1000.f,100.f});
        Vector_PushBack(walls,&(SDL_FRect){400.f,600.f,100.f,25.f});
    }
    SDL_Init(SDL_INIT_EVERYTHING);
    window=SDL_CreateWindow("Game",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    Wtexture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,1000,800);
    emscripten_set_main_loop(loop,-1,0);
    return 0;
}
