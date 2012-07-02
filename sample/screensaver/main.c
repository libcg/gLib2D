// The famous DVD screensaver.

#include <pspkernel.h>
#include "../../glib2d.h"
#include "../callbacks.h"

PSP_MODULE_INFO("App",0,1,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main()
{
  callbacks_setup();

  int size = 42, x=0, y=0, dx=2, dy=2;

  while (1)
  {
    x += dx;
    y += dy;
    if (x < 0 || x+size > G2D_SCR_W) dx = -dx;
    if (y < 0 || y+size > G2D_SCR_H) dy = -dy;

    g2dClear(WHITE);

    g2dBeginRects(NULL); // No texture
    g2dSetColor(AZURE);
    g2dSetScaleWH(size,size);
    g2dSetCoordXY(x,y);
    g2dSetAlpha(x*255/G2D_SCR_W); // Useless alpha effect ;)
    g2dAdd();
    g2dEnd();

    g2dFlip(G2D_VSYNC);
  }

  sceKernelExitGame();
  return 0;
}
