// Control object properties.

#include <pspkernel.h>
#include <pspctrl.h>
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
  
  SceCtrlData pad;
  gImage* tex = gTexLoad("tex.png",G_TRUE);
  int alpha = 255, x = G_SCR_W/2, y = G_SCR_H/2,
      w = tex->w, h = tex->h, rot = 0;

  while (1)
  {
    // Controls
    sceCtrlPeekBufferPositive(&pad,1);
    if (pad.Buttons & PSP_CTRL_SELECT)   alpha-=2;
    if (pad.Buttons & PSP_CTRL_START)    alpha+=2;
    if (pad.Buttons & PSP_CTRL_LEFT)     x-=2;
    if (pad.Buttons & PSP_CTRL_RIGHT)    x+=2;
    if (pad.Buttons & PSP_CTRL_UP)       y-=2;
    if (pad.Buttons & PSP_CTRL_DOWN)     y+=2;
    if (pad.Buttons & PSP_CTRL_SQUARE)   w--;
    if (pad.Buttons & PSP_CTRL_CIRCLE)   w++;
    if (pad.Buttons & PSP_CTRL_TRIANGLE) h--;
    if (pad.Buttons & PSP_CTRL_CROSS)    h++;
    if (pad.Buttons & PSP_CTRL_LTRIGGER) rot-=2;
    if (pad.Buttons & PSP_CTRL_RTRIGGER) rot+=2;
    
    // Display
    gClear(WHITE);
    
    gBegin(tex);
    if (tex == NULL) gSetColor(RED);
    gSetCoordMode(G_CENTER);
    gSetAlpha(alpha);
    gSetScaleWH(w,h);
    gSetCoordXY(x,y);
    gSetRotation(rot);
    gAdd();
    gEnd();
    
    gFlip(G_TRUE); // Vsync enabled
  }
    
  sceKernelExitGame();
  return 0;
}
