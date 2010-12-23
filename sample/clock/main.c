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
  
  gBeginLines(G_STRIP);
  
  gSetCoordXY(G_SCR_W/2,G_SCR_H/2);
  gSetColor(LITEGRAY);
  
  for (i=0; i!=n+1; i++)
  {
    gPush();
    gSetCoordXYRelative(0.f,-size);
    gAdd();
    gPop();
    
    gSetRotationRelative(360.f/n);
  }  
  
  gEnd();
}


void drawClockHands()
{
  gBeginLines(G_VOID);
  
  gSetCoordXY(G_SCR_W/2,G_SCR_H/2);

  // Hours
  gPush();
  gSetColor(BLACK);
  gSetRotation((time.hour%12+
                time.minutes/60.f+
                time.seconds/3600.f)*360.f/12.f);
  gAdd();
  gSetCoordXYRelative(0.f,-30.f);
  gAdd();
  gPop();
  
  // Minutes
  gPush();
  gSetColor(BLACK);
  gSetRotation((time.minutes+
                time.seconds/60.f)*360.f/60.f);
  gAdd();
  gSetCoordXYRelative(0.f,-70.f);
  gAdd();
  gPop();
  
  // Seconds
  gPush();
  gSetColor(RED);
  gSetAlpha(255);
  gSetRotation(time.seconds*360.f/60.f);
  gAdd();
  gSetCoordXYRelative(0.f,-70.f);
  gSetAlpha(100);
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
    
    drawBorder();
    drawClockHands();
    
    gFlip(G_VSYNC);
  }
    
  sceKernelExitGame();
  return 0;
}
