#include "mathlib.h"
#include "render.h"
#include "microui.h"


namespace draw
{

  void DrawQuad(size_t size, Uivertex* vertices, size_t index_count, uint16_t* index_array);
  void DrawIndexedTriangle(size_t size, Vertex_* vertices, size_t index_count, uint32_t* index_array);
  void PresentUI();
 int r_get_text_width(const char *text, int len);
 int text_width(mu_Font font, const char *text, int len); 
 int text_height(mu_Font font);
 void r_draw_rect(mu_Rect rect, mu_Color color);

} //namespace draw
