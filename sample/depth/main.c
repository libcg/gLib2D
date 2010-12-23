// Zbuffer use.
// Z range : nearest 0 - 65535 farest
// Objects are automatically sorted to have a proper rendering with blending.

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
  
  while (1)
  {
    gClear(BLACK); // Clears zbuffer since Z coordinate is used in the loop
    
    gBeginRects(NULL);
    
    gSetColor(AZURE);
    gSetAlpha(255);
    gSetScaleWH(50,50);
    gSetCoordXYZ(195,50,0);
    gAdd();

    gSetColor(CHARTREUSE);
    gSetAlpha(200);
    gSetScaleWH(200,100);
    gSetCoordXYZ(20,20,0);
    gAdd();

    gSetColor(0xFFDDEEFF);
    gSetAlpha(127);
    gSetScaleWH(100,67);
    gSetCoordXYZ(170,60,1);
    gAdd();
    
    gEnd();
    
    gFlip(G_VSYNC);
  }
    
  sceKernelExitGame();
  return 0;
}
