// Quads and lines use.

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
