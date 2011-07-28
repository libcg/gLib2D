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
  g2dImage* tex = g2dTexLoad("tex.png",G2D_SWIZZLE);
  int alpha = 255,
      x = G2D_SCR_W/2,
      y = G2D_SCR_H/2,
      w = (tex == NULL ? 10 : tex->w),
      h = (tex == NULL ? 10 : tex->h),
      rot = 0;

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
    g2dClear(WHITE);
    
    g2dBeginRects(tex);
    if (tex == NULL) g2dSetColor(RED);
    g2dSetCoordMode(G2D_CENTER);
    g2dSetAlpha(alpha);
    g2dSetScaleWH(w,h);
    g2dSetCoordXY(x,y);
    g2dSetRotation(rot);
    g2dAdd();
    g2dEnd();
    
    g2dFlip(G2D_VSYNC);
  }
    
  sceKernelExitGame();
  return 0;
}
