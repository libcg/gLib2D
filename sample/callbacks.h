#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <pspkernel.h>

int callbacks_exit()
{
    sceKernelExitGame();

    return 0;
}
  
int callbacks_thread()
{
    int id;
    
    id = sceKernelCreateCallback("exit_cb", callbacks_exit, NULL);
    sceKernelRegisterExitCallback(id);
    sceKernelSleepThreadCB();
    
    return 0;
}
  
int callbacks_setup()
{
    int id;
    
    id = sceKernelCreateThread("cb", callbacks_thread, 0x11, 0xFA0, 0, NULL);
    
    if (id >= 0)
    {
        sceKernelStartThread(id, 0, NULL);
    }
    
    return id;
}

#endif // __CALLBACKS_H__
