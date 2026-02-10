#define SDL_MAIN_HANDLED
#include <emscripten/html5.h>
#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include "vec.c"
#include "vid.h"

//random comment

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
SDL_Surface* bloodstain;
Video* plr_anim;
Video* plr_animWithSwordIdle;
Video* plr_animWithSwordCalm;
Vector* plr_animWithSwordAttack;
Video* plr_animWithSwordMidAir;
Video* plr_animLegsWalking;
Vector* plr_animWithSwordAttacks;
void(*lastloop) () ;
void(*currloop) () ;
void switch_loop(void(*to) ()) {
  lastloop=currloop;
  currloop=to;
} 
float dt,acc;
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

typedef struct BloodParticle{
    Sprite base;
    float dx,dy;
    float lifetime;
} BloodParticle;

void BloodParticle_update(void* obj){
    BloodParticle* o=(BloodParticle*)obj;
    o->lifetime-=dt;
    if (o->lifetime<=0.f){ 
        o->base.active=0;
        emscripten_log(EM_LOG_CONSOLE,"BloodParticle lifetime ended");
        return;
    }
    float weight=1.f;
    o->dy-=gravity*dt*weight;
    o->base.rect->x+=o->dx;
    o->base.rect->y-=o->dy;
    VECTOR_FOR(o->base.collisions,i,SDL_FRect){
        if (SDL_HasIntersectionF(o->base.rect,i)){
            if (o->base.rect->y<=i->y){
                o->base.rect->y=i->y-o->base.rect->h;
            }
            else{
                o->base.rect->y=i->y+i->h;
            }
            o->dy=0;
        }
    }
    SDL_RenderCopyF(renderer,o->base.txt,NULL,o->base.rect);
}

void BloodParticle_destroy(void* obj){
    BloodParticle* o=(BloodParticle*)obj;
    SDL_DestroyTexture(o->base.txt);
    free(o->base.rect);
    free(o);
}

BloodParticle* BloodParticle_create(float dy,float dx,SDL_FRect rect,Vector* sprites,Vector* collisions){
    BloodParticle* o=malloc(sizeof(BloodParticle));
    if (!o){
        emscripten_log(EM_LOG_ERROR,"Allocation failed");
        return NULL;
    }
    {
        SDL_FRect* r=malloc(sizeof(SDL_FRect));
        *r=rect;
        o->base.rect=r;
    }
    o->dx=dx;
    o->dy=dy;
    o->base.hp=-1;
    o->base.txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,o->base.rect->w,o->base.rect->h);
    void* pixels;
    int pitch;
    SDL_LockTexture(o->base.txt,NULL,&pixels,&pitch);
    for (int i=0;i<o->base.rect->h;i++){
        Uint32* row=(Uint32*)((Uint8*)pixels+(pitch*i));
        for (int j=0;j<o->base.rect->w;j++){
            row[j]=0xFF0000FF;
        }
    }
    SDL_UnlockTexture(o->base.txt);
    o->base.sprites=sprites;
    o->base.collisions=collisions;
    o->base.active=1;
    o->lifetime=3.f;
    o->base.destroy=BloodParticle_destroy;
    o->base.update=BloodParticle_update;
    return o;
}

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
        SDL_TEXTUREACCESS_STREAMING,(int)o->base.rect->w,(int)o->base.rect->h);
    void* pixels;
    int pitch;
    SDL_LockTexture(o->base.txt,NULL,&pixels,&pitch);
    for (int i=0;i<(int)o->base.rect->h;i++){
        Uint32* row=(Uint32*)((Uint8*)pixels+(pitch*i));
        for (int j=0;j<(int)o->base.rect->w;j++){
            row[j]=0xFF0000FF;
        }
    }
    SDL_UnlockTexture(o->base.txt);
    o->base.active=1;
    o->base.hp=-1;
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
    float paramCd;
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
            a=PuddleOfBlood_create((SDL_FRect){o->base.rect->x,
                o->base.rect->y+o->base.rect->h-10,o->base.rect->w,10});
            SDL_DestroyTexture(a->base.txt);
            a->base.txt=SDL_CreateTextureFromSurface(renderer,bloodstain);
            *a->base.rect=*o->base.rect;
            Vector_PushBack(o->base.sprites,&a);
            BloodParticle* tmp=BloodParticle_create(5.f,50.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,25.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,10.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,-25.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,-10.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,-50.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,-20.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            tmp=BloodParticle_create(5.f,20.f,
                (SDL_FRect){o->base.rect->x,o->base.rect->y,10.f,10.f},
                o->base.sprites,o->base.collisions);
            Vector_PushBack(o->base.sprites,&tmp);
            VECTOR_FOR(o->base.sprites,i,Sprite*){
                Sprite* s=*i;
                emscripten_log(EM_LOG_CONSOLE,
                "sprite=%p update=%p destroy=%p",
                s, s->update, s->destroy);
            }
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

    if (SDL_HasIntersectionF(o->target,o->base.rect) && o->cd<=0.f) {
        *o->hp-=o->damage;
        o->cd=o->paramCd;
    }
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
    o->speed=150.f;
    o->inAir=0;
    o->base.hp=50;
    o->base.active=1;
    o->base.collisions=collisions;
    o->base.sprites=sprites;
    o->target=target;
    o->cd=0.f;
    o->weight=1.f;
    o->paramCd=1.f;
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

#define DIR_NONE 0
#define DIR_UP 1
#define DIR_UP_RIGHT 2
#define DIR_RIGHT 3
#define DIR_DOWN_RIGHT 4
#define DIR_DOWN 5
#define DIR_DOWN_LEFT 6
#define DIR_LEFT 7
#define DIR_UP_LEFT 8

typedef struct Weapon{
    Sprite* owner;
    void(*update)(void*);
    void(*onFire)(void*,Vector*);
    void(*asItem)(void*,SDL_FPoint*);
    void(*destroy)(void*);
} Weapon;

Vector* LoadPoses(const char* path){
    emscripten_log(EM_LOG_CONSOLE,"Loading poses from %s",path);    
    Vector* res=CreateVector(sizeof(Video*));
    Vector_Resize(res,9);
    char buf[256];
    Vector_PushBack(res,&(Video*){NULL});
    snprintf(buf,256,"%s/Up",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/UpRight",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/Right",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/DownRight",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/Down",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/DownLeft",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/Left",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    snprintf(buf,256,"%s/UpLeft",path);
    Vector_PushBack(res,&(Video*){Video_create(buf,renderer,6,&dt)});
    return res;
}

typedef struct Sword{
    Weapon base;
    float timeAttacking;
    int damage;
    int animOn;
    float cd;
    int* moving;
    int* midAir;
    int lastLoops;
    Vector* attacks_pos;
    Video* legsAnim;
    Video* Idle;
    Video* Calm;
    Video* MidAir;
    int* dir;
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
    SDL_FRect dmgRect={ownerRect.x+ownerRect.w/2,ownerRect.y+ownerRect.h/2,ownerRect.w,ownerRect.h};
    switch (*o->dir){
        case DIR_UP:
          dmgRect=(SDL_FRect){ownerRect.x,ownerRect.y+20,ownerRect.w,20};
          break;
        case DIR_UP_RIGHT:
          dmgRect=(SDL_FRect){ownerRect.x+ownerRect.w/2,ownerRect.y+20,ownerRect.w,20};
          break;
        case DIR_UP_LEFT:
          dmgRect=(SDL_FRect){ownerRect.x-ownerRect.w/2,ownerRect.y+20,ownerRect.w,20};
          break;
        case DIR_DOWN:
          dmgRect=(SDL_FRect){ownerRect.x,ownerRect.y-ownerRect.h,ownerRect.w,20};
          break;
        case DIR_DOWN_RIGHT:
          dmgRect=(SDL_FRect){ownerRect.x+ownerRect.w/2,ownerRect.y-ownerRect.h,ownerRect.w,20};
          break;
        case DIR_DOWN_LEFT:
          dmgRect=(SDL_FRect){ownerRect.x-ownerRect.w/2,ownerRect.y-ownerRect.h,ownerRect.w,20};
          break;
        case DIR_RIGHT:
          dmgRect=(SDL_FRect){ownerRect.x+ownerRect.w,ownerRect.y,20,ownerRect.h};
          break;
        case DIR_LEFT:
          dmgRect=(SDL_FRect){ownerRect.x-20,ownerRect.y,20,ownerRect.h};
          break;
        default:
            return;
    }
    int j=0;
    int backup_hp=o->base.owner->hp;
    VECTOR_FOR(ens,i,Sprite*){
        if (SDL_HasIntersectionF(&dmgRect,(*i)->rect) && (*i)->hp>0) {
            (**i).hp-=o->damage;
        }
        j++;
    }
    o->base.owner->hp=backup_hp;
    o->animOn=1;
    o->timeAttacking=0.5f;
}

void Sword_update(void* obj){
    if (!obj) return;
    Sword* o=(Sword*)obj;
    SDL_FRect ownerRect=*(o->base.owner->rect);
    SDL_FRect drawRect={ownerRect.x+(ownerRect.w/2.f)-75.f,ownerRect.y+(ownerRect.h/2.f),
        75.f,10.f};
    if (o->animOn){
        o->timeAttacking-=dt;
        if (o->timeAttacking<=0.f){
            o->timeAttacking=0.f;
            o->animOn=0;
        }
    }

    for (int i=0;i<Vector_Size(o->attacks_pos);i++){
        Video** vv = Vector_Get(o->attacks_pos,i);
        emscripten_log(EM_LOG_CONSOLE,"ptr=%p val=%p",vv,*vv);
    }

    if (o->animOn){
        emscripten_log(EM_LOG_CONSOLE,"anim on branch dir %d",*o->dir);
        Video_setPos(o->MidAir,0);
        Video_setPos(o->Calm,0);
        Video_setPos(o->Idle,0);
        emscripten_log(EM_LOG_CONSOLE,"Before setting attacking poses");
        for (int i=1;i<Vector_Size(o->attacks_pos);i++){
            if (*o->dir!=i)
                Video_setPos(*(Video**)Vector_Get(o->attacks_pos,i),0);
        }
        emscripten_log(EM_LOG_CONSOLE,"After setting attacking poses");
        Video_update(*(Video**)Vector_Get(o->attacks_pos,*o->dir));
        Video_update(o->legsAnim);
        emscripten_log(EM_LOG_CONSOLE,"pos %d",*o->dir);
        if (*o->moving)
            SDL_RenderCopyF(renderer,
                Video_getFrame(o->legsAnim),
                NULL,o->base.owner->rect);
        else
            SDL_RenderCopyF(renderer,
                Vector_Get(o->legsAnim->frames,1),
                NULL,o->base.owner->rect);
        if (*o->dir!=0)
            SDL_RenderCopyF(renderer,
                Video_getFrame(*(Video**)Vector_Get(o->attacks_pos,*o->dir)),
                NULL,o->base.owner->rect);
    }
    else if (*o->midAir){
        emscripten_log(EM_LOG_CONSOLE,"midAir branch");
        Video_setPos(o->Calm,0);
        Video_setPos(o->Idle,0);
        Video_setPos(o->legsAnim,0);
        emscripten_log(EM_LOG_CONSOLE,"Before setting attacking poses");
        for (int i=1;i<Vector_Size(o->attacks_pos);i++){
            Video_setPos(*(Video**)Vector_Get(o->attacks_pos,i),0);
        }
        emscripten_log(EM_LOG_CONSOLE,"After setting attacking poses");
        Video_update(o->MidAir);
        SDL_RenderCopyF(renderer,
            Video_getFrame(o->MidAir),
            NULL,o->base.owner->rect);
    }
    else if (*o->moving){
        emscripten_log(EM_LOG_CONSOLE,"moving branch");
        Video_setPos(o->Calm,0);
        Video_setPos(o->Idle,0);
        Video_setPos(o->legsAnim,0);
        emscripten_log(EM_LOG_CONSOLE,"Before setting attacking poses");
        for (int i=1;i<Vector_Size(o->attacks_pos);i++){
            Video_setPos(*(Video**)Vector_Get(o->attacks_pos,i),0);
        }
        emscripten_log(EM_LOG_CONSOLE,"After setting attacking poses");
        Video_update(o->Idle);
        SDL_RenderCopyF(renderer,
            Video_getFrame(o->Idle),
            NULL,o->base.owner->rect);
    }
    else{
        emscripten_log(EM_LOG_CONSOLE,"idle branch");
        Video_setPos(o->Calm,0);
        Video_setPos(o->MidAir,0);
        Video_setPos(o->legsAnim,0);
        emscripten_log(EM_LOG_CONSOLE,"Before setting attacking poses");
        for (int i=1;i<Vector_Size(o->attacks_pos);i++){
            Video_setPos(*(Video**)Vector_Get(o->attacks_pos,i),0);
        }
        emscripten_log(EM_LOG_CONSOLE,"After setting attacking poses");
        Video_update(o->Calm);
        SDL_RenderCopyF(renderer,
            Video_getFrame(o->Calm),
            NULL,o->base.owner->rect);
    }
}

void Sword_asItem(void* obj,SDL_FPoint* point){
    Sword* o=(Sword*)obj;
    SDL_FRect rect={point->x,point->y,10,75};
    SDL_RenderCopyF(renderer,o->txt,NULL,&rect);
}

Vector* CopyVideosShallow(Vector* v){
    Vector* res=CreateVector(sizeof(Video*));
    Vector_Resize(res,Vector_Size(v));
    for (int i=0;i<Vector_Size(v);i++){
        emscripten_log(EM_LOG_CONSOLE,"Loop %d",i);
        Video* t=Video_CopyShallow(*(Video**)Vector_Get(v,i));
        Vector_PushBack(res,&t);
    }
    return res;
}

void* Sword_create(Sprite* owner,int* moving,int* midAir,int* dir){
    Sword* res=malloc(sizeof(Sword));
    emscripten_log(EM_LOG_CONSOLE,"Before copying poses");
    //res->attacks_pos=CopyVideosShallow(plr_animWithSwordAttacks);
    res->attacks_pos=plr_animWithSwordAttacks;
    emscripten_log(EM_LOG_CONSOLE,"Overall %d vectors",Vector_Size(res->attacks_pos));
    res->dir=dir;
    res->moving=moving;
    res->midAir=midAir;
    res->timeAttacking=0.f;
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
    res->legsAnim=Video_CopyShallow(plr_animLegsWalking);
    res->Idle=Video_CopyShallow(plr_animWithSwordIdle);
    res->Calm=Video_CopyShallow(plr_animWithSwordCalm);
    res->MidAir=Video_CopyShallow(plr_animWithSwordMidAir);
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
    if (dt>0.016f)
        dt=0.016f;
    end=start;
}

float plr_dy=0.f;
float plr_dx=0.f;
int plr_inAir=1;
float plr_speed=300.f;
float plr_dshSpeed=100.f;
int plr_canDash=1;
float plr_weight=1.f;
float plr_jmpPower=7.5f;
int plr_hp=100;
int fullscreen=0;

SDL_FRect plr={0.f,0.f,75.f,75.f};
Sprite plr_sprite={.rect=&plr};
Weapon* plr_wep;
Vector* walls;
Vector* sprites;
float RedScreenAlpha=0.f;
int lastHp;
int mx,my;
int pressed=0;
int plr_walking=0;
int plr_dir=DIR_NONE;

void GameOver(void){
    quit();
}

void loop(void){

    HandleDelta();

    if (plr_hp<lastHp) RedScreenAlpha=255;
    lastHp=plr_hp;

    RedScreenAlpha-=dt*255;
    RedScreenAlpha=SDL_max(RedScreenAlpha,0.f);
  
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type==SDL_QUIT)
            quit();
        if (e.type==SDL_KEYDOWN){
            if (e.key.keysym.sym==SDLK_r)
                emscripten_log(EM_LOG_CONSOLE,"rect at %.2f %.2f",plr.x,plr.y);
        }
        if (e.type==SDL_KEYDOWN){
            if (e.key.keysym.sym==SDLK_f){
                if (fullscreen==0){
                    emscripten_request_fullscreen("#canvas",1);
                    fullscreen=1;
                }
                else{
                    emscripten_exit_fullscreen();
                    fullscreen=0;
                }
            }
        }
        if (e.type==SDL_MOUSEBUTTONDOWN){
            if (e.button.button==SDL_BUTTON_LEFT){
                emscripten_log(EM_LOG_CONSOLE,"Mouse at %d %d",e.button.x,e.button.y);

                mx = e.button.x-300;
                my = e.button.y;

                pressed = 1;
            }
        }
    }

    const Uint8* keys=SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]){
        if(!plr_inAir){
            plr_dy=plr_jmpPower;
            plr_inAir=1;
        }
    }

    plr_dx=0.f;

    plr_walking=0;

    if (keys[SDL_SCANCODE_A]){
        plr_dx=plr_speed*dt*-1;
        plr_walking=1;
    }
    if (keys[SDL_SCANCODE_D]){
        plr_dx=plr_speed*dt;
        plr_walking=1;
    }
    if (keys[SDL_SCANCODE_E] && plr_canDash){
        plr_canDash=0;
        plr_dx=plr_dshSpeed*(plr_dx<0 ? -1 : 1);
    }
    if (!keys[SDL_SCANCODE_E]){
        plr_canDash=1;
    }
    Uint32 mstate=SDL_GetMouseState(NULL,NULL);
    if (keys[SDL_SCANCODE_F]){
        plr_wep->onFire((void*)plr_wep,sprites);
    }

    plr_dir=DIR_NONE;

    if (keys[SDL_SCANCODE_UP]){
      if (keys[SDL_SCANCODE_RIGHT])
        plr_dir=DIR_UP_RIGHT;
      else if (keys[SDL_SCANCODE_LEFT])
        plr_dir=DIR_UP_LEFT;
      else
          plr_dir=DIR_UP;
    }
    if (keys[SDL_SCANCODE_DOWN]){
      if (keys[SDL_SCANCODE_RIGHT])
        plr_dir=DIR_DOWN_RIGHT;
      else if (keys[SDL_SCANCODE_LEFT])
        plr_dir=DIR_DOWN_LEFT;
      else
          plr_dir=DIR_DOWN;
    }
    if (keys[SDL_SCANCODE_RIGHT])
        plr_dir=DIR_RIGHT;
    else if (keys[SDL_SCANCODE_LEFT])
        plr_dir=DIR_LEFT;

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
        SDL_SetRenderDrawColor(renderer,100,100,100,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_SetRenderDrawColor(renderer,150,150,150,255);
        VECTOR_FOR(walls,i,SDL_FRect){
            SDL_RenderFillRectF(renderer,i);
        }

        for (int i=0;i<Vector_Size(sprites);i++){
            Sprite* o=*(Sprite**)Vector_Get(sprites,i);
            if (o->update && o){
                o->update(o);
            }
        }

        int j=0;
        for (int i = Vector_Size(sprites) - 1; i >= 0; --i){
            Sprite* s = *(Sprite**)Vector_Get(sprites, i);
            if (!s->active && (s && s->destroy)){
                s->destroy(s);
                Vector_erase(sprites,i);
            }
        }
        
        emscripten_log(EM_LOG_CONSOLE,"Before updating weapon");
        plr_wep->update((void*)plr_wep);
        emscripten_log(EM_LOG_CONSOLE,"After updating weapon");
        if (plr_hp<=0) GameOver();
        SDL_SetRenderDrawColor(renderer,255,0,0,RedScreenAlpha);
        SDL_RenderFillRect(renderer,NULL);
    }

    SDL_SetRenderTarget(renderer,NULL);
    if (pressed){
        SDL_FRect target={mx,my,10.f,10.f};
        SDL_RenderFillRectF(renderer,&target);
        pressed=0;
    }
    SDL_RenderCopy(renderer,Wtexture,NULL,NULL);

    SDL_RenderPresent(renderer);
}

void init1(){
    lastHp=plr_hp;
    plr_wep=Sword_create(&plr_sprite,&plr_walking,&plr_inAir,&plr_dir);
    walls=CreateVector(sizeof(SDL_FRect));
    sprites=CreateVector(sizeof(Sprite*));
    Vector_Resize(sprites,1024);
    {
        Vector_PushBack(walls,&(SDL_FRect){0.f,700.f,1000.f,100.f});
        Vector_PushBack(walls,&(SDL_FRect){400.f,600.f,100.f,25.f});
    }
    {
        Sprite* tmp=(Sprite*)Enemy_create(&plr,(SDL_FRect){100,300,100,100}
            ,walls,sprites,&plr_hp);
        Vector_PushBack(sprites,&tmp);
        tmp=(Sprite*)Enemy_create(&plr,(SDL_FRect){900,300,100,100}
            ,walls,sprites,&plr_hp);
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
    IMG_Init(IMG_INIT_PNG);
    window=SDL_CreateWindow("Game",0,0,1000,800,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Wtexture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,1000,800);
    bloodstain=IMG_Load("assets/bloodstain.png");
    plr_anim=Video_create("assets/plr_anim",renderer,12,&dt);
    plr_animWithSwordIdle=Video_create(
        "assets/plr_animWithSwordIdle",renderer,6,&dt);
    plr_animWithSwordAttack=CreateVector(sizeof(Video*));
    plr_animWithSwordCalm=Video_create(
        "assets/plr_animWithSwordCalm",renderer,6,&dt);
    plr_animWithSwordMidAir=Video_create(
        "assets/plr_animWithSwordMidAir",renderer,6,&dt);
    plr_animLegsWalking=Video_create(
        "assets/plr_animLegsWalking",renderer,12,&dt);
    emscripten_log(EM_LOG_CONSOLE,"legs anim length %d",
        Vector_Size(plr_animLegsWalking->frames));
    plr_animWithSwordAttacks=LoadPoses("assets/plr_animWithSwordAttacks");
    emscripten_log(EM_LOG_CONSOLE,"Right poses length %d",
        Vector_Size((*(Video**)Vector_Get(plr_animWithSwordAttacks,DIR_RIGHT))->frames));
    start=SDL_GetTicks();
    end=SDL_GetTicks();

    init1();

    emscripten_log(EM_LOG_CONSOLE,"Loop started");
    currloop=loop;
    emscripten_set_main_loop(currloop,-1,0);
    return 0;
}
