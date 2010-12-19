// Quads and lines use.

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
    
    // Draw the quad
    
    gBeginQuads(NULL); // Can be also textured
    
    gSetColor(RED);
    gSetCoordXY(1*G_SCR_W/4,1*G_SCR_H/4);
    gPush();
    gSetRotation(rot);
    gSetCoordXYRelative(45,0,G_TRUE);
    gAdd();
    gPop();
    
    gSetColor(GREEN);
    gSetCoordXY(3*G_SCR_W/4,1*G_SCR_H/4);
    gPush();
    gSetRotation(2.f*rot);
    gSetCoordXYRelative(55,0,G_TRUE);
    gAdd();
    gPop();
    
    gSetColor(BLUE);
    gSetCoordXY(3*G_SCR_W/4,3*G_SCR_H/4);
    gPush();
    gSetRotation(-rot);
    gSetCoordXYRelative(23,0,G_TRUE);
    gAdd();
    gPop();
    
    gSetColor(BLACK);
    gSetCoordXY(1*G_SCR_W/4,3*G_SCR_H/4);
    gPush();
    gSetRotation(-3.f*rot);
    gSetCoordXYRelative(30,0,G_TRUE);
    gAdd();
    gPop();
    
    gEnd();
    
    // Then the rotating lines.
    
    gBeginLines();
    gSetColor(GRAY);
    
    gSetCoordXY(1*G_SCR_W/4,1*G_SCR_H/4);
    gAdd();
    gPush();
    gSetRotation(rot);
    gSetCoordXYRelative(45,0,G_TRUE);
    gAdd();
    gPop();
    
    gSetCoordXY(3*G_SCR_W/4,1*G_SCR_H/4);
    gAdd();
    gPush();
    gSetRotation(2.f*rot);
    gSetCoordXYRelative(55,0,G_TRUE);
    gAdd();
    gPop();
    
    gSetCoordXY(3*G_SCR_W/4,3*G_SCR_H/4);
    gAdd();
    gPush();
    gSetRotation(-rot);
    gSetCoordXYRelative(23,0,G_TRUE);
    gAdd();
    gPop();
    
    gSetCoordXY(1*G_SCR_W/4,3*G_SCR_H/4);
    gAdd();
    gPush();
    gSetRotation(-3.f*rot);
    gSetCoordXYRelative(30,0,G_TRUE);
    gAdd();
    gPop();
    
    gEnd();
    
    gFlip(G_TRUE); // Vsync enabled
  }
    
  sceKernelExitGame();
  return 0;
}
