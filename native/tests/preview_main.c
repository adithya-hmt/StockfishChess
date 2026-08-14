#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"

void sf_render_preview_screen(uint32_t *pixels, int width, int height, int stride, int screen);

void ANativeActivity_setWindowFlags(ANativeActivity*a,uint32_t b,uint32_t c){(void)a;(void)b;(void)c;}
void ANativeActivity_setWindowFormat(ANativeActivity*a,int32_t b){(void)a;(void)b;}
int32_t ANativeWindow_getWidth(ANativeWindow*a){(void)a;return 360;}
int32_t ANativeWindow_getHeight(ANativeWindow*a){(void)a;return 800;}
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow*a,int32_t b,int32_t c,int32_t d){(void)a;(void)b;(void)c;(void)d;return 0;}
int32_t ANativeWindow_lock(ANativeWindow*a,ANativeWindow_Buffer*b,ARect*c){(void)a;(void)b;(void)c;return -1;}
int32_t ANativeWindow_unlockAndPost(ANativeWindow*a){(void)a;return 0;}
ALooper *ALooper_forThread(void){return 0;}
ALooper *ALooper_prepare(int a){(void)a;return 0;}
int ALooper_addFd(ALooper*a,int b,int c,int d,int(*e)(int,int,void*),void*f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return 0;}
int ALooper_removeFd(ALooper*a,int b){(void)a;(void)b;return 0;}
void AInputQueue_attachLooper(AInputQueue*a,ALooper*b,int c,int(*d)(int,int,void*),void*e){(void)a;(void)b;(void)c;(void)d;(void)e;}
void AInputQueue_detachLooper(AInputQueue*a){(void)a;}
int32_t AInputQueue_getEvent(AInputQueue*a,AInputEvent**b){(void)a;(void)b;return -1;}
int32_t AInputQueue_preDispatchEvent(AInputQueue*a,AInputEvent*b){(void)a;(void)b;return 0;}
void AInputQueue_finishEvent(AInputQueue*a,AInputEvent*b,int c){(void)a;(void)b;(void)c;}
int32_t AInputEvent_getType(const AInputEvent*a){(void)a;return 0;}
int32_t AMotionEvent_getAction(const AInputEvent*a){(void)a;return 0;}
float AMotionEvent_getX(const AInputEvent*a,size_t b){(void)a;(void)b;return 0;}
float AMotionEvent_getY(const AInputEvent*a,size_t b){(void)a;(void)b;return 0;}

static int write_ppm(const char *path, const uint32_t *pixels, int w, int h, int stride){
    FILE *f=fopen(path,"wb"); int x,y; if(!f)return 0; fprintf(f,"P6\n%d %d\n255\n",w,h);
    for(y=0;y<h;++y)for(x=0;x<w;++x){uint32_t p=pixels[y*stride+x];unsigned char rgb[3]={(unsigned char)(p&255u),(unsigned char)((p>>8)&255u),(unsigned char)((p>>16)&255u)};fwrite(rgb,1,3,f);}fclose(f);return 1;
}
int main(int argc,char**argv){int screen=argc>2?atoi(argv[2]):5,w=360,h=800;uint32_t *p=calloc((size_t)w*h,4);if(!p)return 2;sf_render_preview_screen(p,w,h,w,screen);if(!write_ppm(argc>1?argv[1]:"preview.ppm",p,w,h,w))return 3;free(p);return 0;}
