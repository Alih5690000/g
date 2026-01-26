#include <emscripten/html5.h>
#include <SDL.h>
#include "vec.c"

void quit(){
    emscripten_cancel_main_loop();
}

float dt;
int start,end;

void HandleDelta(){
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
}

SDL_Texture* Wtexture;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_FRect plr={0.f,0.f,50.f,50.f};
float plr_dy=0.f;
float plr_dx=0.f;
int plr_inAir=1;
float plr_speed=300.f;
float gravity=900.f;
float plr_dshSpeed=100.f;
int plr_canDash=1;
Vector* walls;

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
    }

    SDL_SetRenderTarget(renderer,NULL);
    SDL_RenderCopy(renderer,Wtexture,NULL,NULL);

    SDL_RenderPresent(renderer);
}

int main(){
    walls=CreateVector(sizeof(SDL_FRect));
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
