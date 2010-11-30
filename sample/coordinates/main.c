// gSetCoordMode use.
// The rotation center are coordinates passed to gSetCoord*.

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
    
    gClear(WHITE);
    
    gBeginRects(NULL);
    gSetScaleWH(42,42);
    gSetColor(RED);
    
    gSetCoordMode(G_UP_LEFT);
    gSetCoordXY(0,0); 
    gSetRotation(rot);
    gAdd();

    gSetCoordMode(G_UP_RIGHT);
    gSetCoordXY(G_SCR_W,0);
    gSetRotation(-rot);
    gAdd();

    gSetCoordMode(G_DOWN_RIGHT);
    gSetCoordXY(G_SCR_W,G_SCR_H);
    gSetRotation(rot);
    gAdd();

    gSetCoordMode(G_DOWN_LEFT);
    gSetCoordXY(0,G_SCR_H);
    gSetRotation(-rot); 
    gAdd();

    gSetCoordMode(G_CENTER);
    gSetCoordXY(G_SCR_W/2,G_SCR_H/2);
    gSetRotation(rot);
    gAdd();
    
    gEnd();
    
    gFlip(G_TRUE); // Vsync enabled
  }
    
  sceKernelExitGame();
  return 0;
}
