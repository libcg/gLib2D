// A simple clock (using lines).

#include <pspkernel.h>
#include <psprtc.h>
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

pspTime time;

void drawBorder() // A circle.
{
  int i, n = 42;
  float size = 80.f;
  
  gBeginLines();
  
  gSetCoordXY(G_SCR_W/2,G_SCR_H/2);
  gSetColor(LITEGRAY);
  
  for (i=0; i!=n; i++)
  {
    gPush();
    gSetCoordXYRelative(0.f,-size,G_TRUE);
    gAdd();
    gPop();
    
    gSetRotationRelative(360.f/n);
    
    gPush();
    gSetCoordXYRelative(0.f,-size,G_TRUE);
    gAdd();
    gPop();
  }  
  
  gEnd();
}


void drawNeedles()
{
  gBeginLines();
  
  gSetCoordXY(G_SCR_W/2,G_SCR_H/2);

  // Hours
  gPush();
  gSetColor(BLACK);
  gSetRotation((time.hour%12+
                time.minutes/60.f+
                time.seconds/3600.f)*360.f/12.f);
  gAdd();
  gSetCoordXYRelative(0.f,-30.f,G_TRUE);
  gAdd();
  gPop();
  
  // Minutes
  gPush();
  gSetColor(BLACK);
  gSetRotation((time.minutes+
                time.seconds/60.f)*360.f/60.f);
  gAdd();
  gSetCoordXYRelative(0.f,-70.f,G_TRUE);
  gAdd();
  gPop();
  
  // Seconds
  gPush();
  gSetColor(RED);
  gSetRotation(time.seconds*360.f/60.f);
  gAdd();
  gSetCoordXYRelative(0.f,-70.f,G_TRUE);
  gAdd();
  gPop();

  gEnd();
}


int main()
{
  SetupCallbacks();
  
  while (1)
  {
    sceRtcGetCurrentClockLocalTime(&time);
    
    gClear(WHITE);
    
    gBeginQuads(NULL);
    gSetColor(BLACK);
    gSetCoordXY(20,12);
    gAdd();
    gSetColor(AZURE);
    gSetCoordXY(100,5);
    gAdd();
    gSetColor(RED);
    gSetCoordXY(80,45);
    gAdd();
    gSetColor(CHARTREUSE);
    gSetCoordXY(30,70);
    gAdd();
    gEnd();
    
    drawBorder();
    drawNeedles();
    
    gFlip(G_TRUE); // Vsync enabled
  }
    
  sceKernelExitGame();
  return 0;
}
