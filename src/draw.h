#include "startup.h"
#include "mathlib.h"
#include "render.h"
#include "microui.h"
#include "entity.h"

namespace draw
{

extern int buf_idx;

void Quad(size_t size, Uivertex* vertices, size_t index_count, uint16_t* index_array);
void Triangle(size_t size, float4_t* vertices);
void Meshes();
void CameraVectors();
void Line(vec3_t from, vec3_t to, vec3_t color, basic_ent_t& ent);
void PresentUI();
int r_get_text_width(const char *text, int len);
int text_width(mu_Font font, const char *text, int len);
int text_height(mu_Font font);
void InitUI();
void InitAtlasTexture();
void Rect(mu_Rect rect, mu_Color color);
void Text(const char *text, mu_Vec2 pos, mu_Color color);
void Icon(int id, mu_Rect rect, mu_Color color);
void Stats();
void Cursor();
void InitSkydome();
void SkyDome();
} //namespace draw
