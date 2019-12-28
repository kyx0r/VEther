#include "obj_parse.h"
#include "zone.h"
#include "flog.h"
#include<setjmp.h>

#ifdef OBJ_PARSE_IMPLEMENTATION

jmp_buf OBJFatal;

OBJParserArena
OBJParserArenaInit(void *memory, unsigned int memory_size)
{
	OBJParserArena arena = {};
	arena.memory = memory;
	arena.memory_alloc_position = 0;
	arena.memory_size = arena.memory_left = memory_size;
	return arena;
}

void *
OBJParserArenaAllocate(OBJParserArena *arena, unsigned int size)
{
	void *memory = 0;
	if(arena->memory && size <= arena->memory_left)
	{
		memory = (char *)arena->memory + arena->memory_alloc_position;
		arena->memory_alloc_position += size;
		arena->memory_left -= size;
	}
	return memory;
}

char *
OBJParserArenaAllocateCStringCopy(OBJParserArena *arena, char *string)
{
	char *result = 0;
	int string_length = 0;
	for(; string[string_length]; ++string_length);
	result = (char *)OBJParserArenaAllocate(arena, string_length + 1);
	for(int i = 0; i < string_length; ++i)
	{
		result[i] = string[i];
	}
	result[string_length] = 0;
	return result;
}

int
OBJParserCharToLower(int c)
{
	if(c >= 'A' && c <= 'Z')
	{
		c += 32;
	}
	return c;
}

int
OBJParserCharIsAlpha(int c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int
OBJParserCharIsDigit(int c)
{
	return (c >= '0' && c <= '9');
}

int
OBJParserStringMatchCaseSensitive(char *str1, char *str2)
{
	int result = 1;

	if(str1 && str2)
	{
		for(int i = 0;; ++i)
		{
			if(str1[i] != str2[i])
			{
				result = 0;
				break;
			}
			if(str1[i] == 0 && str2[i] == 0)
			{
				break;
			}
		}
	}
	else if(str1 || str2)
	{
		result = 0;
	}

	return result;
}

int
OBJParserStringMatchCaseInsensitiveN(char *str1, char *str2, int n)
{
	int result = 1;

	if(str1 && str2)
	{
		for(int i = 0; i < n; ++i)
		{
			if(OBJParserCharToLower(str1[i]) != OBJParserCharToLower(str2[i]))
			{
				result = 0;
				break;
			}
			if(str1[i] == 0 && str2[i] == 0)
			{
				break;
			}
		}
	}
	else if(str1 || str2)
	{
		result = 0;
	}

	return result;
}

int
OBJTokenMatch(OBJToken token, char *string)
{
	return (OBJParserStringMatchCaseInsensitiveN(token.string, string, token.string_length) &&
	        string[token.string_length] == 0);
}

OBJToken
OBJParserGetNextTokenFromBuffer(char *buffer)
{
	OBJToken token = {};

	enum
	{
		SEARCH_MODE_normal,
		SEARCH_MODE_ignore_line,
	};

	int search_mode = SEARCH_MODE_normal;

	for(int i = 0; buffer[i]; ++i)
	{

		if(search_mode == SEARCH_MODE_normal)
		{
			if(buffer[i] == '#')
			{
				search_mode = SEARCH_MODE_ignore_line;
			}
			else if(OBJParserCharIsAlpha(buffer[i]))
			{
				int j = i+1;
				for(; buffer[j] && (OBJParserCharIsAlpha(buffer[j]) || buffer[j] == '_'); ++j);

				// NOTE(rjf): OBJ tokens

				if(OBJParserStringMatchCaseInsensitiveN("vn", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_vertex_normal_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("vt", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_vertex_uv_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("v", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_vertex_position_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("f", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_face_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("o", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_object_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("g", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_group_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("s", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_shading_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("mtllib", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_material_library_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("usemtl", buffer + i, j - i))
				{
					token.type = OBJ_TOKEN_TYPE_use_material_signifier;
				}

				// NOTE(rjf): MTL tokens

				else if(OBJParserStringMatchCaseInsensitiveN("newmtl", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_new_material_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Ka", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_ambient_color_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Kd", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_diffuse_color_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Ks", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_specular_color_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Ke", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_emissive_color_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Ns", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_specular_exponent_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Ni", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_optical_density_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("d", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_dissolve_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Tr", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_transparency_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("Tf", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_transmission_filter_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("illum", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_illumination_type_signifier;
				}

				// NOTE(rjf): Maps

				else if(OBJParserStringMatchCaseInsensitiveN("map_Ka", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_albedo_map_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("map_Kd", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_diffuse_map_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("map_Ks", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_specular_map_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("map_d", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_dissolve_map_signifier;
				}
				else if(OBJParserStringMatchCaseInsensitiveN("map_bump", buffer + i, j - i))
				{
					token.type = MTL_TOKEN_TYPE_bump_map_signifier;
				}

				token.string = buffer+i;
				token.string_length = j-i;
				break;
			}
			else if(OBJParserCharIsDigit(buffer[i]) || buffer[i] == '-')
			{
				int j = i+1;

				token.type = OBJ_TOKEN_TYPE_number;

				for(; buffer[j] && (OBJParserCharIsDigit(buffer[j]) || buffer[j] == '.' || OBJParserCharIsAlpha(buffer[j])); ++j);

				token.string = buffer+i;
				token.string_length = j-i;
				break;
			}
			else if(buffer[i] == '/')
			{
				token.string = buffer+i;
				token.string_length = 1;
				token.type = OBJ_TOKEN_TYPE_face_index_divider;
				break;
			}
		}
		else if(search_mode == SEARCH_MODE_ignore_line)
		{
			if(buffer[i] == '\n')
			{
				search_mode = SEARCH_MODE_normal;
			}
		}
	}

	return token;
}

OBJToken
OBJNextToken(OBJTokenizer *tokenizer)
{
	OBJToken token = OBJParserGetNextTokenFromBuffer(tokenizer->at);
	if(token.type != OBJ_TOKEN_TYPE_null)
	{
		tokenizer->at = token.string + token.string_length;
	}
	return token;
}

void
OBJSkipUntilNextLine(OBJTokenizer *tokenizer)
{
	for(int i = 0; tokenizer->at[i]; ++i)
	{
		if(tokenizer->at[i] == '\n')
		{
			tokenizer->at += ++i;
			break;
		}
	}
}

void
OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(OBJTokenizer *tokenizer, char *destination, int destination_max)
{
	int destination_write_pos = 0;
	int found_non_whitespace = 0;
	char *one_past_last_non_whitespace_character = tokenizer->at;

	for(int i = 0; tokenizer->at[i] && destination_write_pos < destination_max - 1; ++i)
	{
		if(tokenizer->at[i] == '\n')
		{
			break;
		}
		else
		{

			if(found_non_whitespace)
			{
				destination[destination_write_pos++] = tokenizer->at[i];
				one_past_last_non_whitespace_character = destination + destination_write_pos;
			}
			else
			{
				if(tokenizer->at[i] > 32)
				{
					found_non_whitespace = 1;
					--i;
				}
			}

		}
	}

	*one_past_last_non_whitespace_character = 0;
	destination[destination_write_pos++] = 0;
}

OBJToken
OBJPeekToken(OBJTokenizer *tokenizer)
{
	OBJToken token = OBJParserGetNextTokenFromBuffer(tokenizer->at);
	return token;
}

int
OBJRequireToken(OBJTokenizer *tokenizer, char *string, OBJToken *token_ptr)
{
	int match = 0;
	OBJToken token = OBJPeekToken(tokenizer);
	if(OBJTokenMatch(token, string))
	{
		match = 1;
		tokenizer->at = token.string + token.string_length;
		if(token_ptr)
		{
			*token_ptr = token;
		}
	}
	return match;
}

int
OBJRequireTokenType(OBJTokenizer *tokenizer, OBJTokenType type, OBJToken *token_ptr)
{
	int match = 0;
	OBJToken token = OBJPeekToken(tokenizer);
	if(token.type == type)
	{
		match = 1;
		tokenizer->at = token.string + token.string_length;
		if(token_ptr)
		{
			*token_ptr = token;
		}
	}
	return match;
}

#define OBJParseCStringToInt(i) (atoi(i))
#define OBJParseCStringToFloat(f) ((float)atof(f))

float
OBJTokenToFloat(OBJToken token)
{
	uint32_t float_str_write_pos = 0;
	char float_str[64] = {};
	float val = 0.f;

	for(int i = 0; i < token.string_length; ++i)
	{
		if(token.string[i] == '-' || OBJParserCharIsDigit(token.string[i]))
		{
			for(int j = i; j < token.string_length && token.string[j] &&
			        (token.string[j] == '-' || OBJParserCharIsDigit(token.string[j]) || token.string[j] == '.'); ++j)
			{
				if(float_str_write_pos < sizeof(float_str))
				{
					float_str[float_str_write_pos++] = token.string[j];
				}
				else
				{
					break;
				}
			}
			break;
		}
	}

	val = OBJParseCStringToFloat(float_str);

	return val;
}

int
OBJTokenToInt(OBJToken token)
{
	uint32_t int_str_write_pos = 0;
	char int_str[64] = {};
	int val = 0;

	if(token.string)
	{
		for(int i = 0; i < token.string_length; ++i)
		{
			if(token.string[i] == '-' || OBJParserCharIsDigit(token.string[i]))
			{
				for(int j = i; j < token.string_length && token.string[j] &&
				        (token.string[j] == '-' || OBJParserCharIsDigit(token.string[j])); ++j)
				{
					if(int_str_write_pos < sizeof(int_str))
					{
						int_str[int_str_write_pos++] = token.string[j];
					}
					else
					{
						break;
					}
				}
				break;
			}
		}
		val = OBJParseCStringToInt(int_str);
	}

	return val;
}

#ifndef OBJ_PARSE_NO_CRT

char *
OBJParseLoadEntireFileAndNullTerminate(char *filename, int* size)
{
	char *result = 0;
	FILE *file = fopen(filename, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		unsigned int file_size = ftell(file);
		*size = file_size;
		fseek(file, 0, SEEK_SET);
		result = (char*)zone::Hunk_Alloc(file_size+1);
		if(result)
		{
			fread(result, 1, file_size, file);
			result[file_size] = 0;
		}
		fclose(file);
	}
	else
	{
		error("Failed to open file!  %s", filename);
	}
	return result;
}

void
OBJParseFreeFileData(void *file_data)
{
	free(file_data);
}

void
OBJParseDefaultCRTErrorCallback(OBJParseError error)
{
	error("OBJ PARSE ERROR (%s:%i): %s", error.filename, error.line, error.message);
}

void
OBJParseDefaultCRTWarningCallback(OBJParseWarning warning)
{
	warn("OBJ PARSE WARNING (%s:%i): %s", warning.filename, warning.line, warning.message);
}

ParsedOBJ
LoadOBJ(char *filename)
{

	if (setjmp(OBJFatal))
	{
		fatal("LoadOBJ: out of memory!");
		ASSERT(0, "");
	}

	OBJParserPersistentState *persistent_state = nullptr; 
	int mark = zone::Hunk_LowMark();
	int size;
	OBJParseInfo info = {};
	ParsedOBJ obj = {};
	obj.mark = zone::Hunk_HighMark();
	{
		info.obj_data = OBJParseLoadEntireFileAndNullTerminate(filename, &size);
		info.parse_memory_size = size*10; //give it 10 time as much as of a filesize itself.
		//info.parse_memory = zone::Hunk_HighPos();
		info.parse_memory = zone::Hunk_HighAllocName(info.parse_memory_size, "model");
		if(!info.parse_memory)
		{
			warn("LoadOBJ: Using malloc instead of Hunk!");
			info.parse_memory = malloc(info.parse_memory_size);
			obj.mark = -1;
		}
		info.filename = filename;
		info.error_callback    = OBJParseDefaultCRTErrorCallback;
		info.warning_callback  = OBJParseDefaultCRTWarningCallback;
	}
	if(info.obj_data)
	{
		int tmp = obj.mark;
		obj = ParseOBJ(&info);
		obj.mark = tmp;
		zone::Hunk_FreeToLowMark(mark);
		persistent_state = (OBJParserPersistentState *)((char *)obj.renderables - sizeof(OBJParserPersistentState));
	}

	// NOTE(rjf): Load in material libraries.
	{
		char obj_folder[256] = {};
		snprintf(obj_folder, sizeof(obj_folder), "%s", filename);
		char *one_past_last_slash = obj_folder;
		for(int i = 0; obj_folder[i]; ++i)
		{
			if(obj_folder[i] == '/' || obj_folder[i] == '\\')
			{
				one_past_last_slash = obj_folder + i + 1;
			}
		}
		*one_past_last_slash = 0;

		for(unsigned int i = 0; i < obj.material_library_count; ++i)
		{
			char mtl_filename[256] = {};
			snprintf(mtl_filename, sizeof(mtl_filename), "%s%s", obj_folder, obj.material_libraries[i]);
			LoadMTLForOBJ(mtl_filename, &obj);
		}
	}
	//reclaim any extra allocated space.
/*	if(obj.mark != -1)
	{
		OBJParserArena *arena = &persistent_state->parser_arena;
		info("Objparse: arena used = %d  |  arena max = %d", arena->memory_alloc_position, info.parse_memory_size);
		persistent_state->parse_memory_to_free = zone::Hunk_ShrinkHigh(info.parse_memory_size - arena->memory_alloc_position);
		obj.renderables->vertices = (obj.renderables->vertices - (float*)info.parse_memory) + (float*)persistent_state->parse_memory_to_free;
		obj.renderables->indices = (obj.renderables->indices - (int*)info.parse_memory) + (int*)persistent_state->parse_memory_to_free;
		p("%p   %p",obj.renderables->vertices,  info.parse_memory);

	}
	else */
	{
		persistent_state->parse_memory_to_free = info.parse_memory;
	}
	return obj;
}

void
LoadMTLForOBJ(char *filename, ParsedOBJ *obj)
{
	OBJParserPersistentState *persistent_state = (OBJParserPersistentState *)((char *)obj->renderables - sizeof(OBJParserPersistentState));
	int mark = zone::Hunk_LowMark();
	int size;
	MTLParseInfo info = {};
	{
		info.mtl_data = OBJParseLoadEntireFileAndNullTerminate(filename, &size);
		info.filename = filename;
		info.error_callback = persistent_state->ErrorCallback;
		info.warning_callback = persistent_state->WarningCallback;
	}

	if(info.mtl_data)
	{
		ParseMTLForOBJ(&info, obj);
		zone::Hunk_FreeToLowMark(mark);
	}
}

void
FreeParsedOBJ(ParsedOBJ *obj)
{
	if(obj->renderables)
	{
		OBJParserPersistentState *persistent_state = (OBJParserPersistentState *)((char *)obj->renderables - sizeof(OBJParserPersistentState));
		if(persistent_state->parse_memory_to_free && obj->mark == -1)
		{
			free(persistent_state->parse_memory_to_free);
		}
		else
		{
			zone::Hunk_FreeToHighMark(obj->mark);
		}
	}
}

#endif // OBJ_PARSE_NO_CRT

ParsedOBJ
ParseOBJ(OBJParseInfo *info)
{
	char *obj_data                  = info->obj_data;
	void *parse_memory              = info->parse_memory;
	unsigned int parse_memory_size  = info->parse_memory_size;
	//char *filename                  = info->filename ? info->filename : (char*)"";
	OBJParseErrorCallback       *ErrorCallback      = info->error_callback;
	OBJParseWarningCallback     *WarningCallback    = info->warning_callback;

	ParsedOBJ obj_ = {};
	ParsedOBJ *obj = &obj_;
	OBJParserArena arena_ = OBJParserArenaInit(parse_memory, parse_memory_size);
	OBJParserArena *arena = &arena_;
	OBJTokenizer tokenizer_ = {};
	OBJTokenizer *tokenizer = &tokenizer_;
	tokenizer->at = obj_data;

	//xunsigned int floats_per_vertex = 3 + 2 + 3;
	//unsigned int bytes_per_vertex = sizeof(float)*floats_per_vertex;

	// NOTE(rjf): Used for storing a linked list of parsed models in the parsing
	// memory, which can then be converted into a contiguous array for the user.
	typedef struct OBJParsedGeometryGroupListNode OBJParsedGeometryGroupListNode;
	struct OBJParsedGeometryGroupListNode
	{
		unsigned int face_vertex_start;
		unsigned int face_vertex_end;
		unsigned int lowest_position_index;
		OBJParsedGeometryGroupListNode *next;
	};
	typedef struct OBJParsedRenderableListNode OBJParsedRenderableListNode;
	struct OBJParsedRenderableListNode
	{
		ParsedOBJMaterial material;
		OBJParsedGeometryGroupListNode *first_geometry_group;
		OBJParsedGeometryGroupListNode **current_target_geometry_group;
		unsigned int geometry_group_count;
		OBJParsedRenderableListNode *next;
	};
	unsigned int renderable_list_node_count = 0;
	OBJParsedRenderableListNode *first_renderable_list_node = 0;
	OBJParsedRenderableListNode **target_renderable_list_node = &first_renderable_list_node;
	OBJParsedRenderableListNode *active_renderable = 0;
	OBJParsedGeometryGroupListNode *active_geometry_group = 0;

	// NOTE(rjf): Similar to above linked lists, except for material libraries.
	typedef struct OBJParsedMaterialLibraryNode OBJParsedMaterialLibraryNode;
	struct OBJParsedMaterialLibraryNode
	{
		char *name;
		OBJParsedMaterialLibraryNode *next;
	};
	unsigned int material_library_list_node_count = 0;
	OBJParsedMaterialLibraryNode *first_material_library_list_node = 0;
	OBJParsedMaterialLibraryNode **target_material_library_list_node = &first_material_library_list_node;

	ParsedOBJMaterial global_material = {};
	ParsedOBJMaterial active_material = global_material;

	// NOTE(rjf): Calculate size of and allocate data for all vertices/UVs/normals read
	// directly from the .obj file.
	float *vertex_positions = 0;
	float *vertex_uvs = 0;
	float *vertex_normals = 0;
	int *face_vertices = 0;
	{
		unsigned int num_positions = 0;
		unsigned int num_uvs = 0;
		unsigned int num_normals = 0;
		unsigned int num_face_vertices = 0;

		for(;;)
		{
			OBJToken token = OBJPeekToken(tokenizer);
			if(token.type == OBJ_TOKEN_TYPE_null)
			{
				break;
			}

			if(OBJRequireToken(tokenizer, "v", 0))
			{
				if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0))
				{
					num_positions += 1;
				}
			}
			else if(OBJRequireToken(tokenizer, "vn", 0))
			{
				if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0))
				{
					num_normals += 1;
				}
			}
			else if(OBJRequireToken(tokenizer, "vt", 0))
			{
				if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0))
				{
					num_uvs += 1;
					OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0);
				}
			}
			else if(OBJRequireToken(tokenizer, "f", 0))
			{
				int face_vertex_count = 0;
				for(;;)
				{
					if(OBJPeekToken(tokenizer).type == OBJ_TOKEN_TYPE_number)
					{
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_face_index_divider, 0);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_face_index_divider, 0);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0);
						++num_face_vertices;
						++face_vertex_count;
					}
					else
					{
						break;
					}
				}

				// NOTE(rjf): We will perform a triangulation step on faces if a face
				// is listed with more than 3 vertices. This is the memory usage calculation
				// stage, so we just need to calculate how many more vertices will be used by
				// the triangulation step. The triangulation method used is just the
				// simple fan-triangulation step:
				//
				//
				//        XXXXXXXXXXX
				//      XXXXXXXX     XX         1. Start with 8 vertices
				//    XXXXXXX  XXXXXXX XX
				//  XX  XXXX XX      XXXXXX     2. Connect first vertex with each other non-adjacent vertex
				//  X  X X XX  XX         X
				//  X  X XX XX   XX       X     3. Profit
				//  X  X  X  XX   XX      X
				//  X X   X   XX    XX    X
				//  X X   X    XX    XXX  X
				//  X X   X     XX     XX X
				//  XX    X      XX      XX
				//    XX  X       XX   XX
				//      XXX        XXXX
				//        XXXXXXXXXXX
				//
				// In this simple algorithm, if we have n input vertices, we'll actually
				// be generating 3 * (n - 2) vertices. For example, with n=8 we'll get
				// 6 triangles, or 6*3 = 18 vertices, which fits expectations (see above
				// diagram), as 3 * (n - 2) = 3 * (8 - 2) = 3 * 6 = 18.

				if(face_vertex_count > 3)
				{
					// NOTE(rjf): We've already added n, so just add what we expect minus that.
					int n = face_vertex_count;
					num_face_vertices += (3 * (n - 2)) - n;
				}
			}

			else
			{
				OBJSkipUntilNextLine(tokenizer);
			}
		}

		// NOTE(rjf): Allocate the memory we need for storing the initial vertex information
		// from the OBJ file.
		void *memory = 0;
		{
			unsigned int needed_memory_for_initial_obj_read =
			    ((num_positions * 3) + (num_uvs * 2) + (num_normals * 3)) * sizeof(float) +
			    (num_face_vertices * 3) * sizeof(int);
			memory = OBJParserArenaAllocate(arena, needed_memory_for_initial_obj_read);
			if(!memory)
			{
				// TODO(rjf): ERROR: Out of memory.
				longjmp(OBJFatal, 1);
			}
		}

		vertex_positions = (float *)memory;
		vertex_uvs = vertex_positions + num_positions * 3;
		vertex_normals = vertex_uvs + num_uvs * 2;
		face_vertices = (int *)(vertex_normals + num_normals*3);
	}

	// NOTE(rjf): Do the parse.
	tokenizer->at = info->obj_data;
	unsigned int vertex_position_write_pos    = 0;
	unsigned int vertex_uv_write_pos          = 0;
	unsigned int vertex_normal_write_pos      = 0;
	unsigned int face_vertex_write_pos        = 0;
	{

		// NOTE(rjf): Allocate the first renderable and geometry group.
		{
			OBJParsedRenderableListNode *renderable = (OBJParsedRenderableListNode *)OBJParserArenaAllocate(arena, sizeof(*renderable));
			OBJParsedGeometryGroupListNode *geometry_group = (OBJParsedGeometryGroupListNode *)OBJParserArenaAllocate(arena, sizeof(*geometry_group));
			if(!renderable || !geometry_group)
			{
				// TODO(rjf): ERROR: Out of memory.
				longjmp(OBJFatal, 1);
			}
			renderable->material.material_name = 0;
			renderable->first_geometry_group = geometry_group;
			renderable->current_target_geometry_group = &renderable->first_geometry_group->next;
			renderable->geometry_group_count = 1;
			geometry_group->face_vertex_start = 0;
			geometry_group->face_vertex_end = 0;
			geometry_group->lowest_position_index = 0;
			geometry_group->next = 0;
			renderable->next = 0;

			*target_renderable_list_node = renderable;
			target_renderable_list_node = &(*target_renderable_list_node)->next;
			++renderable_list_node_count;

			active_renderable = renderable;
			active_geometry_group = geometry_group;
		}

		for(;;)
		{
			OBJToken token = OBJPeekToken(tokenizer);

			if(token.type == OBJ_TOKEN_TYPE_null)
			{
				break;
			}

			// NOTE(rjf): Handle material library listings at the global level.
			else if(token.type == OBJ_TOKEN_TYPE_material_library_signifier)
			{
				OBJNextToken(tokenizer);
				char material_library_path[256] = {};
				OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, material_library_path, sizeof(material_library_path));
				OBJSkipUntilNextLine(tokenizer);

				if(material_library_path[0])
				{
					OBJParsedMaterialLibraryNode *material_library =
					    (OBJParsedMaterialLibraryNode *)OBJParserArenaAllocate(arena, sizeof(OBJParsedMaterialLibraryNode));
					material_library->name = OBJParserArenaAllocateCStringCopy(arena, material_library_path);
					if(!material_library)
					{
						// TODO(rjf): ERROR: Out of memory.
						longjmp(OBJFatal, 1);
					}
					material_library->next = 0;
					*target_material_library_list_node = material_library;
					target_material_library_list_node = &(*target_material_library_list_node)->next;
					++material_library_list_node_count;
				}
			}

			// NOTE(rjf): Handle material usage listings at the global level.
			else if(token.type == OBJ_TOKEN_TYPE_use_material_signifier)
			{
				OBJNextToken(tokenizer);
				char material_name[256] = {};
				OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, material_name, sizeof(material_name));
				OBJSkipUntilNextLine(tokenizer);
				global_material.material_name = OBJParserArenaAllocateCStringCopy(arena, material_name);
				active_material = global_material;

				if(active_renderable)
				{
					if(active_renderable->material.material_name)
					{
						// NOTE(rjf): Find out if this material is already being used by a renderable.
						int is_unseen_material = 1;
						OBJParsedRenderableListNode *renderable_with_material = 0;
						for(OBJParsedRenderableListNode *node = first_renderable_list_node; node; node = node->next)
						{
							if(OBJParserStringMatchCaseSensitive(material_name, node->material.material_name))
							{
								is_unseen_material = 0;
								renderable_with_material = node;
								break;
							}
						}

						OBJParsedGeometryGroupListNode *geometry_group = (OBJParsedGeometryGroupListNode *)OBJParserArenaAllocate(arena, sizeof(*geometry_group));
						geometry_group->face_vertex_start = face_vertex_write_pos;
						geometry_group->face_vertex_end = face_vertex_write_pos;
						geometry_group->lowest_position_index = 0;
						geometry_group->next = 0;

						if(is_unseen_material)
						{
							// NOTE(rjf): Allocate a new renderable for this material.
							{
								OBJParsedRenderableListNode *renderable = (OBJParsedRenderableListNode *)
								        OBJParserArenaAllocate(arena, sizeof(*renderable));
								if(!renderable)
								{
									// TODO(rjf): ERROR: Out of memory.
									longjmp(OBJFatal, 1);
								}
								renderable->material = active_material;
								renderable->first_geometry_group = geometry_group;
								renderable->current_target_geometry_group = &renderable->first_geometry_group->next;
								renderable->geometry_group_count = 1;
								renderable->next = 0;

								*target_renderable_list_node = renderable;
								target_renderable_list_node = &(*target_renderable_list_node)->next;
								++renderable_list_node_count;

								active_renderable = renderable;
								active_geometry_group = geometry_group;
							}
						}
						else
						{
							// NOTE(rjf): This material has already been used by a renderable, so we want to
							// add a new geometry group to that renderable.
							{
								active_renderable = renderable_with_material;
								active_geometry_group = geometry_group;
								*(renderable_with_material->current_target_geometry_group) = geometry_group;
								renderable_with_material->current_target_geometry_group = &(*renderable_with_material->current_target_geometry_group)->next;
								++renderable_with_material->geometry_group_count;
							}
						}
					}
					else
					{
						active_renderable->material = active_material;
					}
				}
			}

			// NOTE(rjf): Vertex position
			else if(OBJRequireToken(tokenizer, "v", 0))
			{
				OBJToken p1;
				OBJToken p2;
				OBJToken p3;
				if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &p1) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &p2) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &p3))
				{
					vertex_positions[vertex_position_write_pos++] = OBJTokenToFloat(p1);
					vertex_positions[vertex_position_write_pos++] = OBJTokenToFloat(p2);
					vertex_positions[vertex_position_write_pos++] = OBJTokenToFloat(p3);
				}
			}

			// NOTE(rjf): Vertex UV
			else if(OBJRequireToken(tokenizer, "vt", 0))
			{
				OBJToken u;
				OBJToken v;
				OBJToken w;
				if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &u) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &v))
				{
					vertex_uvs[vertex_uv_write_pos++] = OBJTokenToFloat(u);
					vertex_uvs[vertex_uv_write_pos++] = OBJTokenToFloat(v);
					OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &w);
				}
			}

			// NOTE(rjf): Vertex normal
			else if(OBJRequireToken(tokenizer, "vn", 0))
			{
				OBJToken n1;
				OBJToken n2;
				OBJToken n3;
				if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &n1) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &n2) &&
				        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &n3))
				{
					vertex_normals[vertex_normal_write_pos++] = OBJTokenToFloat(n1);
					vertex_normals[vertex_normal_write_pos++] = OBJTokenToFloat(n2);
					vertex_normals[vertex_normal_write_pos++] = OBJTokenToFloat(n3);
				}
			}

			// NOTE(rjf): Face definition
			else if(OBJRequireToken(tokenizer, "f", 0))
			{
				int face_vertex_count = 0;
				int *this_face_vertices_ptr = face_vertices + face_vertex_write_pos*3;
				unsigned int last_face_vertex_write_pos = face_vertex_write_pos;
				unsigned int actual_vertices_added = 0;

				for(;;)
				{
					OBJToken position = {};
					OBJToken uv = {};
					OBJToken normal = {};

					if(OBJPeekToken(tokenizer).type == OBJ_TOKEN_TYPE_number)
					{
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &position);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_face_index_divider, 0);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &uv);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_face_index_divider, 0);
						OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &normal);

						int position_index = OBJTokenToInt(position);
						int uv_index       = OBJTokenToInt(uv);
						int normal_index   = OBJTokenToInt(normal);

						if(position_index < 0)
						{
							position_index = (int)vertex_position_write_pos/3 + position_index + 1;
						}
						if(uv_index < 0)
						{
							uv_index = (int)vertex_uv_write_pos/2 + uv_index + 1;
						}
						if(normal_index < 0)
						{
							normal_index = (int)vertex_normal_write_pos/3 + normal_index + 1;
						}

						face_vertices[face_vertex_write_pos*3 + 0] = position_index;
						face_vertices[face_vertex_write_pos*3 + 1] = uv_index;
						face_vertices[face_vertex_write_pos*3 + 2] = normal_index;

						if(active_geometry_group->lowest_position_index == 0 || (uint32_t)position_index < active_geometry_group->lowest_position_index)
						{
							active_geometry_group->lowest_position_index = position_index;
						}

						++face_vertex_write_pos;
						++face_vertex_count;
						++actual_vertices_added;
					}
					else
					{
						break;
					}
				}

				// NOTE(rjf): We will perform a triangulation step on faces if a face
				// is listed with more than 3 vertices.
				//
				//
				//        XXXXXXXXXXX
				//      XXXXXXXX     XX         1. Start with 8 vertices
				//    XXXXXXX  XXXXXXX XX
				//  XX  XXXX XX      XXXXXX     2. Connect first vertex with each other non-adjacent vertex
				//  X  X X XX  XX         X
				//  X  X XX XX   XX       X     3. Profit
				//  X  X  X  XX   XX      X
				//  X X   X   XX    XX    X
				//  X X   X    XX    XXX  X
				//  X X   X     XX     XX X
				//  XX    X      XX      XX
				//    XX  X       XX   XX
				//      XXX        XXXX
				//        XXXXXXXXXXX

				if(face_vertex_count > 3)
				{
					// HACK(rjf): Right now we're going to go ahead and assume that
					// we'll never have a face with 256 vertices, which would be
					// ridiculous. This logic will prevent such a face from crashing
					// the parser, but will not correctly triangulate it. Seriously,
					// though, why do you have a face with 256 vertices?

					if(face_vertex_count > 256)
					{
						face_vertex_count = 256;
					}

					face_vertex_write_pos = last_face_vertex_write_pos;
					int this_face_vertices[256*3];

					for(int i = 0; i < face_vertex_count; ++i)
					{
						this_face_vertices[i*3 + 0] = this_face_vertices_ptr[i*3 + 0];
						this_face_vertices[i*3 + 1] = this_face_vertices_ptr[i*3 + 1];
						this_face_vertices[i*3 + 2] = this_face_vertices_ptr[i*3 + 2];
					}

					actual_vertices_added = 0;

					// NOTE(rjf): We start at 2, because we're skipping both the
					// first vertex listed, AND the one immediately following it
					// (adjacent to the first vertex).
					for(int i = 2; i < face_vertex_count; ++i)
					{
						int triangle_p1[3] =
						{
							this_face_vertices[0],
							this_face_vertices[1],
							this_face_vertices[2],
						};

						int triangle_p2[3] =
						{
							this_face_vertices[(i-1)*3+0],
							this_face_vertices[(i-1)*3+1],
							this_face_vertices[(i-1)*3+2],
						};

						int triangle_p3[3] =
						{
							this_face_vertices[(i)*3+0],
							this_face_vertices[(i)*3+1],
							this_face_vertices[(i)*3+2],
						};

						face_vertices[face_vertex_write_pos*3 + 0] = triangle_p1[0];
						face_vertices[face_vertex_write_pos*3 + 1] = triangle_p1[1];
						face_vertices[face_vertex_write_pos*3 + 2] = triangle_p1[2];
						++face_vertex_write_pos;
						++actual_vertices_added;

						face_vertices[face_vertex_write_pos*3 + 0] = triangle_p2[0];
						face_vertices[face_vertex_write_pos*3 + 1] = triangle_p2[1];
						face_vertices[face_vertex_write_pos*3 + 2] = triangle_p2[2];
						++face_vertex_write_pos;
						++actual_vertices_added;

						face_vertices[face_vertex_write_pos*3 + 0] = triangle_p3[0];
						face_vertices[face_vertex_write_pos*3 + 1] = triangle_p3[1];
						face_vertices[face_vertex_write_pos*3 + 2] = triangle_p3[2];
						++face_vertex_write_pos;
						++actual_vertices_added;
					}

				}


				if(active_geometry_group)
				{
					active_geometry_group->face_vertex_end += actual_vertices_added;
				}

			}

			// NOTE(rjf): Anything else (ignore)
			else
			{
				OBJNextToken(tokenizer);
				OBJSkipUntilNextLine(tokenizer);
			}


		}

	}

	// NOTE(rjf): Allocate the persistent state before the model array, so we
	// can access it by taking the base of the models array and subtracting
	// sizeof(OBJParserPersistentState) bytes.
	OBJParserPersistentState *persistent_state = 0;
	{
		persistent_state = (OBJParserPersistentState *)OBJParserArenaAllocate(arena, sizeof(OBJParserPersistentState));
		if(!persistent_state)
		{
			// TODO(rjf): ERROR: Out of memory.
			longjmp(OBJFatal, 1);
		}
	}

	// NOTE(rjf): Make renderables.
	{
		obj->renderables = (ParsedOBJRenderable *)OBJParserArenaAllocate(arena, renderable_list_node_count*sizeof(ParsedOBJRenderable));

		if(!obj->renderables)
		{
			// TODO(rjf): ERROR: Out of memory.
			longjmp(OBJFatal, 1);
		}

		obj->renderable_count = 0;

		for(OBJParsedRenderableListNode *node = first_renderable_list_node;
		        node; node = node->next)
		{
			ParsedOBJRenderable *renderable = obj->renderables + obj->renderable_count;
			++obj->renderable_count;

			renderable->material = node->material;

			typedef struct VertexUVAndNormalIndices VertexUVAndNormalIndices;
			struct VertexUVAndNormalIndices
			{
				int position_index;
				int uv_index;
				int normal_index;
			};

			typedef struct GeometryGroupFinalData GeometryGroupFinalData;
			struct GeometryGroupFinalData
			{
				unsigned int num_unique_vertices;
				unsigned int num_face_vertices_with_duplicates;
				unsigned int face_vertex_count;
				int *face_vertices;
				VertexUVAndNormalIndices *vertex_uv_and_normal_indices_buffer;
				unsigned int lowest_position_index;
			};

			GeometryGroupFinalData *geometry_groups_final_data = (GeometryGroupFinalData *)OBJParserArenaAllocate(arena, sizeof(GeometryGroupFinalData)*node->geometry_group_count);
			if(!geometry_groups_final_data)
			{
				// TODO(rjf): ERROR: Out of memory.
				longjmp(OBJFatal, 1);
			}

			unsigned int renderable_total_unique_vertices = 0;
			unsigned int renderable_total_face_vertices_with_duplicates = 0;

			unsigned int group_node_index = 0;
			for(OBJParsedGeometryGroupListNode *geometry_group = node->first_geometry_group;
			        geometry_group; geometry_group = geometry_group->next, ++group_node_index)
			{
				int *geometry_group_face_vertices = face_vertices + geometry_group->face_vertex_start*3;
				unsigned int geometry_group_face_vertex_count = geometry_group->face_vertex_end - geometry_group->face_vertex_start;
				VertexUVAndNormalIndices *vertex_uv_and_normal_indices_buffer = 0;
				unsigned int num_face_vertices_with_duplicates = 0;
				unsigned int num_unique_vertices = 0;

				// NOTE(rjf): Finish up vertices and generate final vertex/index buffer.
				{
					// NOTE(rjf): OBJs store indices for positions, normals, and UVs, but OpenGL and
					// other APIs only allow us to have a single index buffer. So, when we get cases
					// like 2/3/4 and 2/3/5, we need to duplicate vertex information. We'll do this
					// in a vertex duplication step, where we'll keep track of position/UV/normal triplets,
					// and in cases where we find duplicates, we'll explicitly allocate storage for
					// duplicates, and fix up the face indices so that they point to the correct
					// indices.
					{
						vertex_uv_and_normal_indices_buffer =
						    (VertexUVAndNormalIndices *)OBJParserArenaAllocate(arena, geometry_group_face_vertex_count * sizeof(VertexUVAndNormalIndices));

						if(!vertex_uv_and_normal_indices_buffer)
						{
							// TODO(rjf): ERROR: Out of memory.
							longjmp(OBJFatal, 1);
						}

						for(unsigned int i = 0; i < geometry_group_face_vertex_count; ++i)
						{
							vertex_uv_and_normal_indices_buffer[i].position_index = 0;
							vertex_uv_and_normal_indices_buffer[i].uv_index = 0;
							vertex_uv_and_normal_indices_buffer[i].normal_index = 0;
						}

						num_face_vertices_with_duplicates = geometry_group_face_vertex_count;
						num_unique_vertices = geometry_group_face_vertex_count;

						for(unsigned int i = 0; i < geometry_group_face_vertex_count; ++i)
						{
							// NOTE(rjf): We subtract 1 because the OBJ spec has 1-based indices.
							int position_index  = geometry_group_face_vertices[i*3 + 0] - 1;
							int uv_index        = geometry_group_face_vertices[i*3 + 1] - 1;
							int normal_index    = geometry_group_face_vertices[i*3 + 2] - 1;

							int geometry_group_position_index = position_index + 1 - geometry_group->lowest_position_index;

							// NOTE(rjf): We've already written to this index, which means this vertex is a duplicate.
							if(vertex_uv_and_normal_indices_buffer[geometry_group_position_index].position_index ||
							        vertex_uv_and_normal_indices_buffer[geometry_group_position_index].uv_index       ||
							        vertex_uv_and_normal_indices_buffer[geometry_group_position_index].normal_index)
							{
								if(vertex_uv_and_normal_indices_buffer[geometry_group_position_index].position_index != position_index+1 ||
								        vertex_uv_and_normal_indices_buffer[geometry_group_position_index].uv_index       != uv_index+1       ||
								        vertex_uv_and_normal_indices_buffer[geometry_group_position_index].normal_index   != normal_index+1)
								{
									// NOTE(rjf): This is a duplicate position, but it has different UV's or normals, so we
									// need to duplicate this vertex.
									{
										VertexUVAndNormalIndices *duplicate = (VertexUVAndNormalIndices *)OBJParserArenaAllocate(arena, sizeof(VertexUVAndNormalIndices));
										if(!duplicate)
										{
											// TODO(rjf): ERROR: Out of memory.
											longjmp(OBJFatal, 1);
										}
										duplicate->position_index  = position_index+1;
										duplicate->uv_index        = uv_index+1;
										duplicate->normal_index    = normal_index+1;
									}

									// NOTE(rjf): Fix up the reference to this position.
									{
										geometry_group_face_vertices[i*3 + 0] = geometry_group->lowest_position_index + num_face_vertices_with_duplicates;
									}

									++num_face_vertices_with_duplicates;
								}
								else
								{
									// NOTE(rjf): Vertex is a complete duplicate, which means we do not need to duplicate it, and can
									// already reference the same information. Decrease the number of unique vertices.
									--num_unique_vertices;
								}
							}
							else
							{
								// NOTE(rjf): We add 1 because the OBJ spec has 1-based indices, and we are checking
								// for 0's.
								vertex_uv_and_normal_indices_buffer[geometry_group_position_index].position_index  = position_index+1;
								vertex_uv_and_normal_indices_buffer[geometry_group_position_index].uv_index        = uv_index+1;
								vertex_uv_and_normal_indices_buffer[geometry_group_position_index].normal_index    = normal_index+1;
							}
						}
					}
				}

				geometry_groups_final_data[group_node_index].num_unique_vertices = num_unique_vertices;
				geometry_groups_final_data[group_node_index].num_face_vertices_with_duplicates = num_face_vertices_with_duplicates;
				geometry_groups_final_data[group_node_index].face_vertex_count = geometry_group_face_vertex_count;
				geometry_groups_final_data[group_node_index].face_vertices = geometry_group_face_vertices;
				geometry_groups_final_data[group_node_index].vertex_uv_and_normal_indices_buffer = vertex_uv_and_normal_indices_buffer;
				geometry_groups_final_data[group_node_index].lowest_position_index = geometry_group->lowest_position_index;

				renderable_total_unique_vertices += num_unique_vertices;
				renderable_total_face_vertices_with_duplicates += num_face_vertices_with_duplicates;
			}

			// NOTE(rjf): Now that we've duplicated vertices where necessary, we can create
			// final vertex and index buffers, where everything is correct for rendering.
			int final_vertex_buffer_write_number = 0;
			unsigned int bytes_needed_for_final_vertex_buffer = sizeof(float) * 8 * renderable_total_unique_vertices;
			float *final_vertex_buffer = (float *)OBJParserArenaAllocate(arena, bytes_needed_for_final_vertex_buffer);
			int final_index_buffer_write_number = 0;
			unsigned int bytes_needed_for_final_index_buffer = sizeof(int) * renderable_total_face_vertices_with_duplicates;
			int *final_index_buffer = (int *)OBJParserArenaAllocate(arena, bytes_needed_for_final_index_buffer);
			if(!final_vertex_buffer || !final_index_buffer)
			{
				// TODO(rjf): ERROR: Out of memory.
				longjmp(OBJFatal, 1);
			}

			unsigned int group_index_offset = 0;

			for(unsigned int group_index = 0; group_index < node->geometry_group_count; ++group_index)
			{
				GeometryGroupFinalData *group_final_data = geometry_groups_final_data + group_index;

				for(unsigned int i = 0; i < group_final_data->face_vertex_count; ++i)
				{
					int position_index = group_final_data->face_vertices[i*3 + 0] - group_final_data->lowest_position_index;
					VertexUVAndNormalIndices *vertex_data = group_final_data->vertex_uv_and_normal_indices_buffer + position_index;

					float position[3] =
					{
						vertex_positions[(vertex_data->position_index-1)*3+0],
						vertex_positions[(vertex_data->position_index-1)*3+1],
						vertex_positions[(vertex_data->position_index-1)*3+2],
					};

					float uv[2] =
					{
						vertex_uvs[(vertex_data->uv_index-1)*2+0],
						vertex_uvs[(vertex_data->uv_index-1)*2+1],
					};

					float normal[3] =
					{
						vertex_normals[(vertex_data->normal_index-1)*3+0],
						vertex_normals[(vertex_data->normal_index-1)*3+1],
						vertex_normals[(vertex_data->normal_index-1)*3+2],
					};

					int geometry_group_position_index = vertex_data->position_index - group_final_data->lowest_position_index + group_index_offset;

					final_vertex_buffer[(geometry_group_position_index)*8+0] = position[0];
					final_vertex_buffer[(geometry_group_position_index)*8+1] = position[1];
					final_vertex_buffer[(geometry_group_position_index)*8+2] = position[2];
					final_vertex_buffer[(geometry_group_position_index)*8+3] = uv[0];
					final_vertex_buffer[(geometry_group_position_index)*8+4] = uv[1];
					final_vertex_buffer[(geometry_group_position_index)*8+5] = normal[0];
					final_vertex_buffer[(geometry_group_position_index)*8+6] = normal[1];
					final_vertex_buffer[(geometry_group_position_index)*8+7] = normal[2];
					final_index_buffer[final_index_buffer_write_number++] = geometry_group_position_index;

					final_vertex_buffer_write_number += 8;
				}

				group_index_offset += group_final_data->num_unique_vertices;
			}

			renderable->vertices = final_vertex_buffer;
			renderable->vertex_count = renderable_total_unique_vertices;
			// NOTE(rjf): This is hard-coded, while we still only support interleaved data.
			renderable->floats_per_vertex = 8;
			renderable->indices = final_index_buffer;
			renderable->index_count = final_index_buffer_write_number;
		}

	}

	// NOTE(rjf): Form the contiguous material libraries array for the user.
	{
		obj->material_libraries = (char **)OBJParserArenaAllocate(arena, sizeof(char *)*material_library_list_node_count);
		if(!obj->material_libraries)
		{
			// TODO(rjf): ERROR: Out of memory.
			longjmp(OBJFatal, 1);
		}
		obj->material_library_count = 0;
		for(OBJParsedMaterialLibraryNode *node = first_material_library_list_node;
		        node; node = node->next)
		{
			obj->material_libraries[obj->material_library_count] = node->name;
			++obj->material_library_count;
		}
	}

	// NOTE(rjf): Fill out state that needs to persist between library calls.
	{
		unsigned int materials_to_load_count = obj->renderable_count;
		persistent_state->materials_to_load = (ParsedOBJMaterial **)OBJParserArenaAllocate(arena, sizeof(ParsedOBJMaterial *) * materials_to_load_count);

		if(!persistent_state->materials_to_load)
		{
			// TODO(rjf): ERROR: Out of memory.
			longjmp(OBJFatal, 1);
		}

		persistent_state->materials_to_load_count = 0;
		for(unsigned int i = 0; i < materials_to_load_count; ++i)
		{
			persistent_state->materials_to_load[persistent_state->materials_to_load_count] = &obj->renderables[i].material;
			++persistent_state->materials_to_load_count;
		}
		persistent_state->parser_arena = arena_;
		persistent_state->ErrorCallback = ErrorCallback;
		persistent_state->WarningCallback = WarningCallback;

		// NOTE(rjf): We'll set this to 0 as the default. The higher-level functions that allocate
		// their own parsing buffers can set this to something else.
		persistent_state->parse_memory_to_free = 0;
	}

	// end_parse:;

	return obj_;
}

void
ParseMTLForOBJ(MTLParseInfo *info, ParsedOBJ *obj)
{
	char *mtl_data                  = info->mtl_data;
	//char *filename                  = info->filename ? info->filename : (char*)"";
	//OBJParseErrorCallback       *ErrorCallback      = info->error_callback;
	//OBJParseWarningCallback     *WarningCallback    = info->warning_callback;

	OBJParserPersistentState *persistent_state = (OBJParserPersistentState *)((char *)obj->renderables - sizeof(OBJParserPersistentState));

	OBJParserArena *arena = &persistent_state->parser_arena;
	OBJTokenizer tokenizer_ = {};
	OBJTokenizer *tokenizer = &tokenizer_;
	tokenizer->at = mtl_data;

	unsigned int materials_to_load_count = persistent_state->materials_to_load_count;
	ParsedOBJMaterial **materials_to_load = persistent_state->materials_to_load;

	if(!obj->renderables)
	{
		longjmp(OBJFatal, 1);
	}

	// NOTE(rjf): Loop through all materials.
	for(;;)
	{
		OBJToken token = OBJPeekToken(tokenizer);
		if(token.type == MTL_TOKEN_TYPE_null && !token.string)
		{
			break;
		}
		else
		{
			if(token.type == MTL_TOKEN_TYPE_new_material_signifier)
			{
				OBJNextToken(tokenizer);

				char material_name[256] = {};
				OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, material_name, sizeof(material_name));
				OBJSkipUntilNextLine(tokenizer);
				ParsedOBJMaterial material = {};
				material.material_name = OBJParserArenaAllocateCStringCopy(arena, material_name);
				if(!material.material_name)
				{
					// TODO(rjf): ERROR: Out of memory.
					longjmp(OBJFatal, 1);
				}

				// NOTE(rjf): Loop through all material attributes.
				for(;;)
				{
					token = OBJPeekToken(tokenizer);
					if(token.type == MTL_TOKEN_TYPE_null || token.type == MTL_TOKEN_TYPE_new_material_signifier)
					{
						break;
					}
					else
					{

						if(token.type == MTL_TOKEN_TYPE_ambient_color_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken red = {};
							OBJToken green = {};
							OBJToken blue = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &red);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &green);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &blue);
							material.albedo_color[0] = OBJTokenToFloat(red);
							material.albedo_color[1] = OBJTokenToFloat(green);
							material.albedo_color[2] = OBJTokenToFloat(blue);
						}

						else if(token.type == MTL_TOKEN_TYPE_diffuse_color_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken red = {};
							OBJToken green = {};
							OBJToken blue = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &red);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &green);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &blue);
							material.diffuse_color[0] = OBJTokenToFloat(red);
							material.diffuse_color[1] = OBJTokenToFloat(green);
							material.diffuse_color[2] = OBJTokenToFloat(blue);
						}

						else if(token.type == MTL_TOKEN_TYPE_specular_color_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken red = {};
							OBJToken green = {};
							OBJToken blue = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &red);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &green);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &blue);
							material.specular_color[0] = OBJTokenToFloat(red);
							material.specular_color[1] = OBJTokenToFloat(green);
							material.specular_color[2] = OBJTokenToFloat(blue);
						}

						else if(token.type == MTL_TOKEN_TYPE_emissive_color_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken red = {};
							OBJToken green = {};
							OBJToken blue = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &red);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &green);
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &blue);
							material.emissive_color[0] = OBJTokenToFloat(red);
							material.emissive_color[1] = OBJTokenToFloat(green);
							material.emissive_color[2] = OBJTokenToFloat(blue);
						}

						else if(token.type == MTL_TOKEN_TYPE_dissolve_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken value = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &value);
							material.opacity = OBJTokenToFloat(value);
						}

						else if(token.type == MTL_TOKEN_TYPE_transparency_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken value = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &value);
							material.opacity = 1 - OBJTokenToFloat(value);
						}

						else if(token.type == MTL_TOKEN_TYPE_illumination_type_signifier)
						{
							OBJNextToken(tokenizer);
							OBJToken value = {};
							OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &value);
							material.illumination_type = OBJTokenToInt(value);
						}

						else if(token.type == MTL_TOKEN_TYPE_albedo_map_signifier)
						{
							OBJNextToken(tokenizer);
							char path[256] = {};
							OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, path, sizeof(path));
							OBJSkipUntilNextLine(tokenizer);
							material.albedo_map_path = OBJParserArenaAllocateCStringCopy(arena, path);
						}

						else if(token.type == MTL_TOKEN_TYPE_diffuse_map_signifier)
						{
							OBJNextToken(tokenizer);
							char path[256] = {};
							OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, path, sizeof(path));
							OBJSkipUntilNextLine(tokenizer);
							material.diffuse_map_path = OBJParserArenaAllocateCStringCopy(arena, path);
						}

						else if(token.type == MTL_TOKEN_TYPE_specular_map_signifier)
						{
							OBJNextToken(tokenizer);
							char path[256] = {};
							OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, path, sizeof(path));
							OBJSkipUntilNextLine(tokenizer);
							material.specular_map_path = OBJParserArenaAllocateCStringCopy(arena, path);
						}

						else if(token.type == MTL_TOKEN_TYPE_dissolve_map_signifier)
						{
							OBJNextToken(tokenizer);
							char path[256] = {};
							OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, path, sizeof(path));
							OBJSkipUntilNextLine(tokenizer);
							material.dissolve_map_path = OBJParserArenaAllocateCStringCopy(arena, path);
						}

						else if(token.type == MTL_TOKEN_TYPE_bump_map_signifier)
						{
							OBJNextToken(tokenizer);
							char path[256] = {};
							OBJReadTrimmedStringUntilNextLineToFixSizedBuffer(tokenizer, path, sizeof(path));
							OBJSkipUntilNextLine(tokenizer);
							material.bump_map_path = OBJParserArenaAllocateCStringCopy(arena, path);
						}

						else
						{
							// TODO(rjf): WARNING: Unexpected token.
							OBJNextToken(tokenizer);
						}

					}
				}

				// NOTE(rjf): For now, we'll do a linear search through all of the materials to
				// load so that we can patch in the values we've parsed. This can probably be
				// done better, but alas, the OBJ format decided to use strings.
				{
					for(unsigned int i = 0; i < materials_to_load_count; ++i)
					{
						if(OBJParserStringMatchCaseSensitive(materials_to_load[i]->material_name, material.material_name))
						{
							*(materials_to_load[i]) = material;
						}
					}
				}

			}
			else
			{
				OBJSkipUntilNextLine(tokenizer);
			}
		}
	}

	//end_parse:;
}

#endif // OBJ_PARSE_IMPLEMENTATION

// The MIT License
//
// Copyright (c) 2019 Ryan Fleury. http://ryanfleury.net
// Copyright (c) 2019 Kyryl Melekhin. https://kyryl.tk
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
