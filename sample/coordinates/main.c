// gSetCoordMode use.
// The rotation center is coordinates passed to gSetCoord*.

#include <pspkernel.h>
#include "../../glib2d.h"

PSP_MODULE_INFO("App",0,1,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Callbacks */
int exit_callback(int arg1, int arg2, void *common) {
  sceKernelExitGame();
  return 0; }
int CallbackThread(SceSize args, void *argp) {
  int cbid;
  cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB();
  return 0; }
int SetupCallbacks() {
  int thid = 0;
  thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
  if(thid >= 0) sceKernelStartThread(thid, 0, 0);
  return thid; }

int main()
{
  SetupCallbacks();
  int rot = 0;
  
  while (1)
  {
    if ((rot++) > 360) rot -= 360;
    
    g2dClear(WHITE);

    g2dBeginRects(NULL);
    g2dSetScaleWH(42,42);
    g2dSetColor(RED);
    
    g2dSetCoordMode(G2D_UP_LEFT);
    g2dSetCoordXY(0,0); 
    g2dSetRotation(rot);
    g2dAdd();

    g2dSetCoordMode(G2D_UP_RIGHT);
    g2dSetCoordXY(G2D_SCR_W,0);
    g2dSetRotation(-rot);
    g2dAdd();

    g2dSetCoordMode(G2D_DOWN_RIGHT);
    g2dSetCoordXY(G2D_SCR_W,G2D_SCR_H);
    g2dSetRotation(rot);
    g2dAdd();

    g2dSetCoordMode(G2D_DOWN_LEFT);
    g2dSetCoordXY(0,G2D_SCR_H);
    g2dSetRotation(-rot); 
    g2dAdd();

    g2dSetCoordMode(G2D_CENTER);
    g2dSetCoordXY(G2D_SCR_W/2,G2D_SCR_H/2);
    g2dSetRotation(rot);
    g2dAdd();
    
    g2dEnd();
    
    g2dFlip(G2D_VSYNC);
  }
    
  sceKernelExitGame();
  return 0;
}
