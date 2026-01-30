#define SDL_MAIN_HANDLED
#include <emscripten/html5.h>
#include <SDL.h>
#include "vec.c"

void SDL_MoveF(SDL_FRect* r,
                float target_x,
                float target_y,
                float speed,
                float dt)
{
    float cx = r->x + r->w * 0.5f;
    float cy = r->y + r->h * 0.5f;

    float dx = target_x - cx;
    float dy = target_y - cy;

    float dist = sqrtf(dx*dx + dy*dy);
    if (dist < 0.001f) return;

    float step = speed * dt;
    if (step > dist) step = dist;

    dx /= dist;
    dy /= dist;

    r->x += dx * step;
    r->y += dy * step;
}

SDL_Texture* Wtexture;
SDL_Renderer* renderer;
SDL_Window* window;
void(*lastloop) () ;
void(*currloop) () ;
void switch_loop(void(*to) ()) {
  lastloop=currloop;
  currloop=to;
} 
float dt;
int start,end;
float gravity=9.f;

void quit(void){
    emscripten_cancel_main_loop();
}

typedef struct Sprite{
    SDL_FRect* rect;
    SDL_Texture* txt;
    Vector* collisions;
    Vector* sprites;
    int hp;
    int active;
    void (*update)(void*);
    void (*destroy)(void*);
    void (*reconstruct)(void*);
} Sprite;

typedef struct PuddleOfBlood{
    Sprite base;
} PuddleOfBlood;

void PuddleOfBlood_update(void* obj){
    PuddleOfBlood* o=(PuddleOfBlood*)obj;
    SDL_RenderCopyF(renderer,o->base.txt,NULL,o->base.rect);
}

void PuddleOfBlood_destroy(void* obj){
    PuddleOfBlood* o=(PuddleOfBlood*)obj;
    free(o->base.rect);
    SDL_DestroyTexture(o->base.txt);
    free(o);
}

PuddleOfBlood* PuddleOfBlood_create(SDL_FRect rect){
    PuddleOfBlood* o=malloc(sizeof(PuddleOfBlood));
    {
        SDL_FRect* copy=malloc(sizeof(SDL_FRect));
        *copy=rect;
        o->base.rect=copy;
    }
    o->base.txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,o->base.rect->w,o->base.rect->h);
    void* pixels;
    int pitch;
    SDL_LockTexture(o->base.txt,NULL,&pixels,&pitch);
    for (int i=0;i<10;i++){
        Uint32* row=(Uint32*)((Uint8*)pixels+(pitch*i));
        for (int j=0;j<o->base.rect->w;j++){
            row[j]=0xFF0000FF;
        }
    }
    SDL_UnlockTexture(o->base.txt);
    o->base.active=1;
    o->base.update=PuddleOfBlood_update;
    o->base.destroy=PuddleOfBlood_destroy;
    return o;
}

typedef struct Enemy{
    Sprite base;
    SDL_FRect* target;
    int inAir;
    int* hp;
    int damage;
    float dy;
    float cd;
    float weight;
    float speed;
} Enemy;

void Enemy_update(void* obj){
    Enemy* o=(Enemy*)obj;
    if (o->base.hp<=0){
        emscripten_log(EM_LOG_CONSOLE,"died");
        o->base.active=0;
        {
            PuddleOfBlood* a=PuddleOfBlood_create((SDL_FRect){o->base.rect->x,
                o->base.rect->y+o->base.rect->h-10,o->base.rect->w,10});
            Vector_PushBack(o->base.sprites,&a);
        }
    }
    if (o->target->x>o->base.rect->x){
        o->base.rect->x+=o->speed*dt;
        if (fabs(o->base.rect->x-o->target->x)<o->speed*dt){
            o->base.rect->x=o->target->x;
        }
        VECTOR_FOR(o->base.collisions,i,SDL_FRect){
            if (SDL_HasIntersectionF(o->base.rect,i))
                o->base.rect->x=i->x-o->base.rect->w;
        }
    }
    else if(o->target->x<o->base.rect->x){
        o->base.rect->x-=o->speed*dt;
        if (fabs(o->base.rect->x-o->target->x)<o->speed*dt){
            o->base.rect->x=o->target->x;
        }
        VECTOR_FOR(o->base.collisions,i,SDL_FRect){
        if (SDL_HasIntersectionF( o->base.rect,i))
            o->base.rect->x=i->x+i->w;
        }
    }

    o->inAir=1;
    o->dy-=gravity*dt*o->weight;
    o->dy=SDL_min(o->dy,0);
    o->base.rect->y-=o->dy;

    VECTOR_FOR(o->base.collisions,i,SDL_FRect){
        if (SDL_HasIntersectionF(o->base.rect,i)){
            if (o->base.rect->y<=i->y){
                o->base.rect->y=i->y-o->base.rect->h;
                o->inAir=0;
            }
            else{
                o->base.rect->y=i->y+i->h;
            }
            o->dy=0;
        }
    }

    if (SDL_HasIntersection(o->target,o->base.rect) && o->cd<=0.f) *o->hp-=o->damage;

    o->cd-=dt;
    o->cd=SDL_max(o->cd,0);
  
    SDL_RenderCopyF(renderer,o->base.txt,NULL,o->base.rect);
}

void Enemy_destroy(void* obj){
    Enemy* o=(Enemy*)obj;
    free(o->base.rect);
    SDL_DestroyTexture(o->base.txt);
    free(o);
}

Enemy* Enemy_create(SDL_FRect* target,SDL_FRect rect,Vector* collisions,Vector* sprites,int* hp){
    Enemy* o=malloc(sizeof(Enemy));
    SDL_FRect* nrect=malloc(sizeof(SDL_FRect));
    *nrect=rect;
    o->base.rect=nrect;
    o->speed=300.f;
    o->inAir=0;
    o->base.hp=50;
    o->base.active=1;
    o->base.collisions=collisions;
    o->base.sprites=sprites;
    o->target=target;
    o->cd=1.f;
    o->weight=1.f;
    o->damage=10;
    o->hp=hp;
    o->base.update=Enemy_update;
    o->base.destroy=Enemy_destroy;
    o->base.txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,75,10);
    void* pixels;
    int pitch;
    SDL_LockTexture(o->base.txt,NULL,&pixels,&pitch);
    for (int i=0;i<10;i++){
        Uint32* row=(Uint32*)((Uint8*)pixels+(pitch*i));
        for (int j=0;j<75;j++){
            row[j]=0xFF00FFFF;
        }
    }
    SDL_UnlockTexture(o->base.txt);
    return o;
}

typedef struct Weapon{
    Sprite* owner;
    void(*update)(void*);
    void(*onFire)(void*,Vector*);
    void(*asItem)(void*,SDL_FPoint*);
    void(*destroy)(void*);
    void(*reconstruct)(void*);
} Weapon;

typedef struct Sword{
    Weapon base;
    float angle;
    int damage;
    int animOn;
    float cd;
    SDL_Texture* txt;
} Sword;

void Sword_destroy(void* obj){
    Sword* o=(Sword*)obj;
    SDL_DestroyTexture(o->txt);
    free(o);
}

//ens vector of Sprite*
void Sword_onFire(void* obj,Vector* ens){
    Sword* o=(Sword*)obj;
    if (o->animOn) return;
    SDL_FRect ownerRect=*(o->base.owner->rect);
    SDL_FRect dmgRect={ownerRect.x-25,ownerRect.y,ownerRect.w+50,ownerRect.h};
    int j=0;
    VECTOR_FOR(ens,i,Sprite*){
        if (SDL_HasIntersectionF(&dmgRect,(*i)->rect) && (*i)->hp>0) {
            (**i).hp-=o->damage;
            emscripten_log(EM_LOG_CONSOLE,"now hp %d",(**i).hp);
        }
        j++;
    }
    o->animOn=1;
    o->angle=360.f;
}

void Sword_update(void* obj){
    Sword* o=(Sword*)obj;
    SDL_FRect ownerRect=*(o->base.owner->rect);
    SDL_FRect drawRect={ownerRect.x+(ownerRect.w/2.f)-75.f,ownerRect.y+(ownerRect.h/2.f),
        75.f,10.f};
    if (o->animOn){
        o->angle-=720*dt;
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

void Sword_reconstruct(void* obj){
    Sword* o=(Sword*)obj;
    o->txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,75,10);
    void* pixels;
    int pitch;
    SDL_LockTexture(o->txt,NULL,&pixels,&pitch);
    for (int i=0;i<10;i++){
        Uint32* row=(Uint32*)((Uint8*)pixels+(pitch*i));
        for (int j=0;j<75;j++){
            row[j]=0x00FF00FF;
        }
    }
    SDL_UnlockTexture(o->txt);
}

void* Sword_create(Sprite* owner){
    Sword* res=malloc(sizeof(Sword));
    res->angle=0.f;
    res->animOn=0;
    res->damage=10;
    res->txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,75,10);
    void* pixels;
    int pitch;
    SDL_LockTexture(res->txt,NULL,&pixels,&pitch);
    for (int i=0;i<10;i++){
        Uint32* row=(Uint32*)((Uint8*)pixels+(pitch*i));
        for (int j=0;j<75;j++){
            row[j]=0x00FF00FF;
        }
    }
    SDL_UnlockTexture(res->txt);
    res->cd=0.f;
    res->base.owner=owner;
    res->base.asItem=Sword_asItem;
    res->base.destroy=Sword_destroy;
    res->base.onFire=Sword_onFire;
    res->base.update=Sword_update;
    res->base.reconstruct=Sword_reconstruct;
    return res;
}

void HandleDelta(){
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
}


float plr_dy=0.f;
float plr_dx=0.f;
int plr_inAir=1;
float plr_speed=300.f;
float plr_dshSpeed=100.f;
int plr_canDash=1;
float plr_weight=1.f;
int plr_hp=100;

SDL_FRect plr={0.f,0.f,50.f,50.f};
Sprite plr_sprite={.rect=&plr};
Weapon* plr_wep;
Vector* walls;
Vector* sprites;
float RedScreenAlpha=0.f;

void GameOver(void){
    quit();
}

void loop(void){

    HandleDelta();

    RedScreenAlpha-=dt*255;
    RedScreenAlpha=SDL_max(RedScreenAlpha,0);
  
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            quit();
        if (e.type==SDL_KEYDOWN){
            if (e.key.keysym.sym==SDLK_r)
                emscripten_log(EM_LOG_CONSOLE,"rect at %.2f %.2f",plr.x,plr.y);
        }
    }

    const Uint8* keys=SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]){
        if(!plr_inAir){
            plr_dy=5.f;
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

    plr_dy-=gravity*dt*plr_weight;
    
    plr.y-=plr_dy;

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
        SDL_SetRenderDrawColor(renderer,100,100,100,255);
        VECTOR_FOR(walls,i,SDL_FRect){
            SDL_RenderFillRectF(renderer,i);
        }
        int j=0;
        VECTOR_FOR(sprites,i,Sprite*){
            if (!(*i)->active){ 
                (*i)->destroy(*i);
                Vector_erase(sprites,j);
                j--;
                i--;
            };
            j++;
        }
        VECTOR_FOR(sprites,i,Sprite*){
            Sprite* s = *i;
            if (s != NULL && s->update != NULL) {
                s->update((void*)s);
            }
        }
        plr_wep->update((void*)plr_wep);
        if (plr_hp<=0) GameOver();
        SDL_SetRenderDrawColor(renderer,255,0,0,RedScreenAlpha);
        SDL_RenderFillRect(renderer,NULL);
    }

    SDL_SetRenderTarget(renderer,NULL);
    SDL_RenderCopy(renderer,Wtexture,NULL,NULL);

    SDL_RenderPresent(renderer);
}

void init1(){
    plr_wep=Sword_create(&plr_sprite);
    walls=CreateVector(sizeof(SDL_FRect));
    sprites=CreateVector(sizeof(Sprite*));
    {
        Vector_PushBack(walls,&(SDL_FRect){0.f,700.f,1000.f,100.f});
        Vector_PushBack(walls,&(SDL_FRect){400.f,600.f,100.f,25.f});
    }
    {
        Sprite* tmp=(Sprite*)Enemy_create(&plr,(SDL_FRect){100,300,100,100},walls,sprites,&plr_hp);
        Vector_PushBack(sprites,&tmp);
    }
    VECTOR_FOR(sprites,i,Sprite*){
        Sprite* s=*i;
        emscripten_log(EM_LOG_CONSOLE,
        "sprite=%p update=%p destroy=%p",
        s, s->update, s->destroy);
    }
}

struct {
  Vector* walls;
  Vector* sprites;
  
} Game2;

int main(){
    emscripten_log(EM_LOG_CONSOLE,"Lets go");
    SDL_Init(SDL_INIT_EVERYTHING);
    window=SDL_CreateWindow("Game",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Wtexture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,1000,800);

    init1();

    emscripten_log(EM_LOG_CONSOLE,"Loop started");
    currloop=loop;
    emscripten_set_main_loop(currloop,-1,0);
    return 0;
}
