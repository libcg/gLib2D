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
    
    g2dClear(WHITE);
    
    // Draw the quad
    
    g2dBeginQuads(NULL); // Can be also textured
    
    g2dSetColor(RED);
    g2dSetCoordXY(1*G2D_SCR_W/4,1*G2D_SCR_H/4);
    g2dPush();
    g2dSetRotation(rot);
    g2dSetCoordXYRelative(45,0);
    g2dAdd();
    g2dPop();
    
    g2dSetColor(GREEN);
    g2dSetCoordXY(3*G2D_SCR_W/4,1*G2D_SCR_H/4);
    g2dPush();
    g2dSetRotation(2.f*rot);
    g2dSetCoordXYRelative(55,0);
    g2dAdd();
    g2dPop();
    
    g2dSetColor(BLUE);
    g2dSetCoordXY(3*G2D_SCR_W/4,3*G2D_SCR_H/4);
    g2dPush();
    g2dSetRotation(-rot);
    g2dSetCoordXYRelative(23,0);
    g2dAdd();
    g2dPop();
    
    g2dSetColor(BLACK);
    g2dSetCoordXY(1*G2D_SCR_W/4,3*G2D_SCR_H/4);
    g2dPush();
    g2dSetRotation(-3.f*rot);
    g2dSetCoordXYRelative(30,0);
    g2dAdd();
    g2dPop();
    
    g2dEnd();
    
    // Then the rotating lines.
    
    g2dBeginLines(G2D_VOID);
    g2dSetColor(GRAY);
    
    g2dSetCoordXY(1*G2D_SCR_W/4,1*G2D_SCR_H/4);
    g2dAdd();
    g2dPush();
    g2dSetRotation(rot);
    g2dSetCoordXYRelative(45,0);
    g2dAdd();
    g2dPop();
    
    g2dSetCoordXY(3*G2D_SCR_W/4,1*G2D_SCR_H/4);
    g2dAdd();
    g2dPush();
    g2dSetRotation(2.f*rot);
    g2dSetCoordXYRelative(55,0);
    g2dAdd();
    g2dPop();
    
    g2dSetCoordXY(3*G2D_SCR_W/4,3*G2D_SCR_H/4);
    g2dAdd();
    g2dPush();
    g2dSetRotation(-rot);
    g2dSetCoordXYRelative(23,0);
    g2dAdd();
    g2dPop();
    
    g2dSetCoordXY(1*G2D_SCR_W/4,3*G2D_SCR_H/4);
    g2dAdd();
    g2dPush();
    g2dSetRotation(-3.f*rot);
    g2dSetCoordXYRelative(30,0);
    g2dAdd();
    g2dPop();
    
    g2dEnd();
    
    g2dFlip(G2D_VSYNC);
  }
    
  sceKernelExitGame();
  return 0;
}
