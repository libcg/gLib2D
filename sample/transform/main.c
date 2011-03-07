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
    
    g2dClear(WHITE);
    
    g2dBeginRects(NULL);
    
    g2dSetScaleWH(15,15);
    g2dSetCoordMode(G2D_CENTER);
    g2dSetCoordXY(G2D_SCR_W/2,G2D_SCR_H/2);
    g2dSetRotation(rot);
    g2dSetColor(AZURE);
    g2dAdd();
    g2dSetRotation(-rot);
    
    for (i=0; i!=branches; i++)
    {
      g2dPush();
      g2dSetAlpha(200);
      g2dSetCoordXYRelative(30,0);
      g2dAdd();
      
      g2dPush();
      g2dSetAlpha(127);
      g2dSetCoordXYRelative(30,-10);
      g2dAdd();
      
      g2dPop();
      g2dSetScaleWH(5,5);
      g2dSetCoordXYRelative(30,10);
      g2dAdd();
      
      g2dPop();
      g2dSetRotationRelative(360/branches);
    }
    
    g2dEnd();
    
    g2dFlip(G2D_VSYNC);
  }
    
  sceKernelExitGame();
  return 0;
}
