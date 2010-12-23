// OpenGL-like transformations.
// gPush saves the current transformation (position/rotation/scale) to the stack
// gPop reads the current transformation from the stack

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
  int rot = 0, i, branches = 4;
  
  while (1)
  {
    if ((rot++) > 360) rot -= 360;
    
    gClear(WHITE);
    
    gBeginRects(NULL);
    
    gSetScaleWH(15,15);
    gSetCoordMode(G_CENTER);
    gSetCoordXY(G_SCR_W/2,G_SCR_H/2);
    gSetRotation(rot);
    gSetColor(AZURE);
    gAdd();
    gSetRotation(-rot);
    
    for (i=0; i!=branches; i++)
    {
      gPush();
      gSetAlpha(200);
      gSetCoordXYRelative(30,0);
      gAdd();
      
      gPush();
      gSetAlpha(127);
      gSetCoordXYRelative(30,-10);
      gAdd();
      
      gPop();
      gSetScaleWH(5,5);
      gSetCoordXYRelative(30,10);
      gAdd();
      
      gPop();
      gSetRotationRelative(360/branches);
    }
    
    gEnd();
    
    gFlip(G_VSYNC); // Vsync enabled
  }
    
  sceKernelExitGame();
  return 0;
}
