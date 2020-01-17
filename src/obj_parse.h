
#ifndef OBJ_PARSE_H_INCLUDED
#define OBJ_PARSE_H_INCLUDED

#ifndef OBJ_PARSE_NO_CRT
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#define PARSED_OBJ_MODEL_FLAG_POSITION   (1<<0)
#define PARSED_OBJ_MODEL_FLAG_UV         (1<<1)
#define PARSED_OBJ_MODEL_FLAG_NORMAL     (1<<2)
#define OBJ_PARSE_IMPLEMENTATION

typedef struct ParsedOBJMaterial ParsedOBJMaterial;
struct ParsedOBJMaterial
{
	char *material_name;
	float albedo_color[3];
	float diffuse_color[3];
	float specular_color[3];
	float emissive_color[3];
	float opacity;
	int illumination_type;
	char *albedo_map_path;
	char *diffuse_map_path;
	char *specular_map_path;
	char *dissolve_map_path;
	char *bump_map_path;
};

typedef struct ParsedOBJRenderable ParsedOBJRenderable;
struct ParsedOBJRenderable
{
	ParsedOBJMaterial material;
	int flags;
	unsigned int vertex_count;
	unsigned int floats_per_vertex;
	float max_vert;
	float *vertices;
	unsigned int index_count;
	int *indices;
};

typedef struct OBJParseArena OBJParseArena;
typedef struct ParsedOBJ ParsedOBJ;
struct ParsedOBJ
{
	int mark;
	unsigned int renderable_count;
	ParsedOBJRenderable *renderables;
	unsigned int material_library_count;
	char **material_libraries;
};

typedef struct OBJParseInfo OBJParseInfo;
typedef struct MTLParseInfo MTLParseInfo;
ParsedOBJ  LoadOBJ(char *filename);
void       LoadMTLForOBJ(char *filename, ParsedOBJ *obj);
ParsedOBJ  ParseOBJ(OBJParseInfo *parse_info);
void       ParseMTLForOBJ(MTLParseInfo *parse_info, ParsedOBJ *obj);
void       FreeParsedOBJ(ParsedOBJ *parsed_obj);

typedef struct OBJParseError    OBJParseError;
typedef struct OBJParseWarning  OBJParseWarning;
typedef void OBJParseErrorCallback      (OBJParseError error);
typedef void OBJParseWarningCallback    (OBJParseWarning warning);

struct OBJParseInfo
{
	char *obj_data;
	void *parse_memory;
	unsigned int parse_memory_size;
	char *filename;
	OBJParseErrorCallback       *error_callback;
	OBJParseWarningCallback     *warning_callback;
};

struct MTLParseInfo
{
	char *mtl_data;
	char *filename;
	OBJParseErrorCallback       *error_callback;
	OBJParseWarningCallback     *warning_callback;
};

enum OBJParseErrorType
{
	OBJ_PARSE_ERROR_TYPE_out_of_memory,
	OBJ_PARSE_ERROR_TYPE_MAX
};
typedef enum OBJParseErrorType OBJParseErrorType;

enum OBJParseWarningType
{
	OBJ_PARSE_WARNING_TYPE_unexpected_token,
	OBJ_PARSE_WARNING_TYPE_MAX
};
typedef enum OBJParseWarningType OBJParseWarningType;

struct OBJParseError
{
	OBJParseErrorType type;
	char *filename;
	int line;
	char *message;
};

struct OBJParseWarning
{
	OBJParseWarningType type;
	char *filename;
	int line;
	char *message;
};

typedef struct OBJParserArena OBJParserArena;
struct OBJParserArena
{
	void *memory;
	unsigned int memory_alloc_position;
	unsigned int memory_size;
	unsigned int memory_left;
};

typedef struct OBJParserPersistentState OBJParserPersistentState;
struct OBJParserPersistentState
{
	OBJParserArena parser_arena;
	unsigned int materials_to_load_count;
	ParsedOBJMaterial **materials_to_load;
	void *parse_memory_to_free;
	OBJParseErrorCallback *ErrorCallback;
	OBJParseWarningCallback *WarningCallback;
};

enum OBJTokenType
{
	OBJ_TOKEN_TYPE_null,
	MTL_TOKEN_TYPE_null = OBJ_TOKEN_TYPE_null,

	OBJ_TOKEN_TYPE_object_signifier,
	OBJ_TOKEN_TYPE_group_signifier,
	OBJ_TOKEN_TYPE_shading_signifier,
	OBJ_TOKEN_TYPE_vertex_position_signifier,
	OBJ_TOKEN_TYPE_vertex_uv_signifier,
	OBJ_TOKEN_TYPE_vertex_normal_signifier,
	OBJ_TOKEN_TYPE_face_signifier,
	OBJ_TOKEN_TYPE_number,
	OBJ_TOKEN_TYPE_face_index_divider,
	OBJ_TOKEN_TYPE_material_library_signifier,
	OBJ_TOKEN_TYPE_use_material_signifier,

	MTL_TOKEN_TYPE_new_material_signifier,                  // newmtl
	MTL_TOKEN_TYPE_ambient_color_signifier,                 // Ka
	MTL_TOKEN_TYPE_diffuse_color_signifier,                 // Kd
	MTL_TOKEN_TYPE_specular_color_signifier,                // Ks
	MTL_TOKEN_TYPE_emissive_color_signifier,                // Ke
	MTL_TOKEN_TYPE_specular_exponent_signifier,             // Ns
	MTL_TOKEN_TYPE_optical_density_signifier,               // Ni
	MTL_TOKEN_TYPE_dissolve_signifier,                      // d
	MTL_TOKEN_TYPE_transparency_signifier,                  // Tr
	MTL_TOKEN_TYPE_transmission_filter_signifier,           // Tf
	MTL_TOKEN_TYPE_illumination_type_signifier,             // illum

	MTL_TOKEN_TYPE_albedo_map_signifier,                    // map_Ka
	MTL_TOKEN_TYPE_diffuse_map_signifier,                   // map_Kd
	MTL_TOKEN_TYPE_specular_map_signifier,                  // map_Ks
	MTL_TOKEN_TYPE_dissolve_map_signifier,                  // map_d
	MTL_TOKEN_TYPE_bump_map_signifier,                      // map_bump
};
typedef enum OBJTokenType OBJTokenType;

typedef struct OBJToken OBJToken;
struct OBJToken
{
	OBJTokenType type;
	char *string;
	int string_length;
};

typedef struct OBJTokenizer OBJTokenizer;
struct OBJTokenizer
{
	char *at;
	char *filename;
	int line;
};

#endif
