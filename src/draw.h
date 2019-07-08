#include "mathlib.h"
#include "render.h"

namespace draw
{

void DrawTriangle(size_t size, Vertex_ *vertices);
void DrawIndexedTriangle(size_t size, Vertex_* vertices, size_t index_count, uint16_t* index_array);

} //namespace draw
