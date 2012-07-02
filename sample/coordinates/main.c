// gSetCoordMode use.
// The rotation center is coordinates passed to gSetCoord*.

#include <pspkernel.h>
#include "../../glib2d.h"
#include "../callbacks.h"

PSP_MODULE_INFO("App",0,1,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main()
{
  callbacks_setup();
  int rot = 0;

  while (1)
  {
    if ((rot++) > 360) rot -= 360;

    g2dClear(WHITE);

    g2dBeginRects(NULL);
    g2dSetScaleWH(42,42);
    g2dSetColor(RED);

    g2dSetCoordMode(G2D_UP_LEFT);
    g2dSetCoordXY(0,0);
    g2dSetRotation(rot);
    g2dAdd();

    g2dSetCoordMode(G2D_UP_RIGHT);
    g2dSetCoordXY(G2D_SCR_W,0);
    g2dSetRotation(-rot);
    g2dAdd();

    g2dSetCoordMode(G2D_DOWN_RIGHT);
    g2dSetCoordXY(G2D_SCR_W,G2D_SCR_H);
    g2dSetRotation(rot);
    g2dAdd();

    g2dSetCoordMode(G2D_DOWN_LEFT);
    g2dSetCoordXY(0,G2D_SCR_H);
    g2dSetRotation(-rot);
    g2dAdd();

    g2dSetCoordMode(G2D_CENTER);
    g2dSetCoordXY(G2D_SCR_W/2,G2D_SCR_H/2);
    g2dSetRotation(rot);
    g2dAdd();

    g2dEnd();

    g2dFlip(G2D_VSYNC);
  }

  sceKernelExitGame();
  return 0;
}
