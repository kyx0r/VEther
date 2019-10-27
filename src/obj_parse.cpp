#include "obj_parse.h"
#include "flog.h"

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

#ifdef OBJ_PARSE_IMPLEMENTATION

typedef struct OBJParserArena OBJParserArena;
struct OBJParserArena
{
	void *memory;
	unsigned int memory_alloc_position;
	unsigned int memory_size;
	unsigned int memory_left;
};

OBJParserArena
OBJParserArenaInit(void *memory, unsigned int memory_size)
{
	OBJParserArena arena;
	memset(&arena, 0, sizeof(OBJParserArena));
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


enum OBJTokenType
{
	OBJ_TOKEN_TYPE_null,
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

int
OBJTokenMatch(OBJToken token, char *string)
{
	return (OBJParserStringMatchCaseInsensitiveN(token.string, string, token.string_length) &&
	        string[token.string_length] == 0);
}

OBJToken
OBJParserGetNextTokenFromBuffer(char *buffer)
{
	OBJToken token;
	memset(&token, 0, sizeof(OBJToken));

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
				for(; buffer[j] && OBJParserCharIsAlpha(buffer[j]); ++j);

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
	unsigned int float_str_write_pos = 0;
	char float_str[64];
	memset(float_str, 0, 64);
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
	unsigned int int_str_write_pos = 0;
	char int_str[64];
	memset(int_str, 0, 64);
	int val = 0;

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

	return val;
}

#ifndef OBJ_PARSE_NO_CRT

char *
OBJParseLoadEntireFileAndNullTerminate(char *filename)
{
	char *result = 0;
	FILE *file = fopen(filename, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		unsigned int file_size = ftell(file);
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
		error("Failed to open file!  %s\n", filename);
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
	error("OBJ PARSE ERROR (%s:%i): %s\n", error.filename, error.line, error.message);
}

void
OBJParseDefaultCRTWarningCallback(OBJParseWarning warning)
{
	warn("OBJ PARSE WARNING (%s:%i): %s\n", warning.filename, warning.line, warning.message);
}

ParsedOBJ
LoadOBJ(char *filename)
{
	int mark = zone::Hunk_LowMark();
	cache_user_t cache;
	cache.data = nullptr;
	char ma = 0;

	OBJParseInfo info;
	memset(&info, 0, sizeof(OBJParseInfo));
	ParsedOBJ obj;
	{
		info.obj_data = OBJParseLoadEntireFileAndNullTerminate(filename);
		info.parse_memory_size = 1024*1024*128;
		if(zone::Cache_Alloc(&cache, info.parse_memory_size, "model"))
		{
			info.parse_memory = cache.data;
		}
		else
		{
			info.parse_memory = malloc(info.parse_memory_size);
			ma = 1;
		}
		if(!info.parse_memory)
		{
			info.parse_memory_size = 0;
		}
		info.filename = filename;
		info.error_callback    = OBJParseDefaultCRTErrorCallback;
		info.warning_callback  = OBJParseDefaultCRTWarningCallback;
	}
	if(info.obj_data)
	{
		obj = ParseOBJ(&info);
		obj.malloc = ma;
		//OBJParseFreeFileData(info.obj_data);
		zone::Hunk_FreeToLowMark(mark);
	}
	obj.parse_memory_to_free = info.parse_memory;
	return obj;
}

void
FreeParsedOBJ(ParsedOBJ *obj)
{
	if(obj->malloc)
	{
		free(obj->parse_memory_to_free);
		return;
	}
	cache_user_t cache;
	cache.data = obj->parse_memory_to_free;
	if(obj->parse_memory_to_free)
	{
		zone::Cache_Free(&cache, false);
	}
}

#endif // OBJ_PARSE_NO_CRT

ParsedOBJ
ParseOBJ(OBJParseInfo *info)
{
	char *obj_data                  = info->obj_data;
	void *parse_memory              = info->parse_memory;
	unsigned int parse_memory_size  = info->parse_memory_size;
	char *filename                  = info->filename ? info->filename : (char*)"";
	//OBJParseErrorCallback       *ErrorCallback      = info->error_callback;
	OBJParseWarningCallback     *WarningCallback    = info->warning_callback;

	ParsedOBJ obj_;
	memset(&obj_, 0, sizeof(ParsedOBJ));
	ParsedOBJ *obj = &obj_;
	OBJParserArena arena_ = OBJParserArenaInit(parse_memory, parse_memory_size);
	OBJParserArena *arena = &arena_;
	OBJTokenizer tokenizer_;
	memset(&tokenizer_, 0, sizeof(OBJTokenizer));
	OBJTokenizer *tokenizer = &tokenizer_;
	tokenizer->at = obj_data;

	//unsigned int floats_per_vertex = 3 + 2 + 3;
	//unsigned int bytes_per_vertex = sizeof(float)*floats_per_vertex;

	typedef struct OBJParsedSubModelListNode OBJParsedSubModelListNode;
	struct OBJParsedSubModelListNode
	{
		int flags;
		unsigned int vertex_count;
		float *vertices;
		unsigned int index_count;
		int *indices;
		ParsedOBJMaterial material;
		OBJParsedSubModelListNode *next;
	};

	typedef struct OBJParsedModelListNode OBJParsedModelListNode;
	struct OBJParsedModelListNode
	{
		OBJParsedSubModelListNode *first_sub_model;
		unsigned int sub_model_count;
		ParsedOBJSubModel *sub_models;
		OBJParsedModelListNode *next;
	};

	unsigned int model_list_node_count = 0;
	OBJParsedModelListNode *first_model_list_node = 0;
	OBJParsedModelListNode **target_model_list_node = &first_model_list_node;

	/* typedef struct OBJParsedMaterialLibraryNode OBJParsedMaterialLibraryNode; */
	/* struct OBJParsedMaterialLibraryNode */
	/* { */
	/*     char *name; */
	/*     OBJParsedMaterialLibraryNode *next; */
	/* }; */

	//unsigned int material_library_list_node_count = 0;
	//OBJParsedModelListNode *first_material_library_list_node = 0;
	//OBJParsedModelListNode **target_material_library_list_node = &first_material_library_list_node;

	// NOTE(rjf): OBJs store per-OBJ indices, NOT per-object or per-group indices,
	// which is kind of a pain. So, we'll store offsets here to correct for that as
	// each object is processed.
	int vertex_position_index_offset = 0;
	int vertex_uv_index_offset = 0;
	int vertex_normal_index_offset = 0;

	//ParsedOBJMaterial global_material = {0};

	// NOTE(rjf): Loop through all objects.
	for(;;)
	{
		OBJToken token = OBJPeekToken(tokenizer);

		if(token.type == OBJ_TOKEN_TYPE_null)
		{
			break;
		}
		else
		{

			// NOTE(rjf): Handle material library listings at the global level.
			if(token.type == OBJ_TOKEN_TYPE_material_library_signifier)
			{
				OBJNextToken(tokenizer);
				OBJSkipUntilNextLine(tokenizer);
			}

			// NOTE(rjf): Handle material usage listings at the global level.
			else if(token.type == OBJ_TOKEN_TYPE_material_library_signifier)
			{
				OBJNextToken(tokenizer);
				OBJSkipUntilNextLine(tokenizer);
			}

			// NOTE(rjf): Shading (ignore for now).
			else if(token.type == OBJ_TOKEN_TYPE_shading_signifier)
			{
				OBJNextToken(tokenizer);
				OBJSkipUntilNextLine(tokenizer);
			}

			// NOTE(rjf): We'll look for an object signifier ("o") to begin parsing
			// vertex information for an object, but if we find a vertex data point
			// before finding a "o" (or a group label), we'll just start forming an
			// object (this might be true in the case where there are no object labels
			// in the OBJ file and so we won't find any). This also provides the arguably
			// most robust solution in the case where there are some vertex attributes
			// before any objects are listed, where it'll just make a new object for the
			// "un-named" vertex attributes.
			else if(token.type == OBJ_TOKEN_TYPE_object_signifier          ||
			        token.type == OBJ_TOKEN_TYPE_group_signifier           ||
			        token.type == OBJ_TOKEN_TYPE_vertex_position_signifier ||
			        token.type == OBJ_TOKEN_TYPE_vertex_uv_signifier       ||
			        token.type == OBJ_TOKEN_TYPE_vertex_normal_signifier)
			{
				// NOTE(rjf): Skip the line if we found an object signifier. We don't want
				// to do this in any other case, since then we'd skip vertex info.
				if(token.type == OBJ_TOKEN_TYPE_object_signifier)
				{
					OBJNextToken(tokenizer);
					OBJSkipUntilNextLine(tokenizer);
				}

				// NOTE(rjf): Allocate a new model.
				OBJParsedModelListNode *model = 0;
				{
					model = (OBJParsedModelListNode*) OBJParserArenaAllocate(arena, sizeof(*model));
					if(!model)
					{
						printf("ERROR: Out of memory.\n");
						goto end_parse;
					}
					model->first_sub_model = 0;
					model->next = 0;
					++model_list_node_count;
				}

				*target_model_list_node = model;
				target_model_list_node = &(*target_model_list_node)->next;
				OBJParsedSubModelListNode **target_sub_model_list_node = &model->first_sub_model;
				unsigned int sub_model_list_node_count = 0;

				// NOTE(rjf): Loop through all polygon groups.
				for(;;)
				{
					OBJToken token = OBJPeekToken(tokenizer);

					// NOTE(rjf): Similar to the object case, if we find vertex information
					// before we find a group label, we'll form an "un-named" polygon group.
					if(token.type == OBJ_TOKEN_TYPE_group_signifier           ||
					        token.type == OBJ_TOKEN_TYPE_vertex_position_signifier ||
					        token.type == OBJ_TOKEN_TYPE_vertex_uv_signifier       ||
					        token.type == OBJ_TOKEN_TYPE_vertex_normal_signifier)
					{
						if(token.type == OBJ_TOKEN_TYPE_group_signifier)
						{
							OBJNextToken(tokenizer);
							OBJSkipUntilNextLine(tokenizer);
						}

						char *object_start = tokenizer->at;

						unsigned int num_positions = 0;
						unsigned int num_uvs = 0;
						unsigned int num_normals = 0;
						unsigned int num_face_vertices = 0;

						// NOTE(rjf): Calculate the number of positions, UVs, and normals that the
						// OBJ file has for this object.
						{
							for(;;)
							{
								token = OBJPeekToken(tokenizer);
								if(token.type != OBJ_TOKEN_TYPE_null &&
								        token.type != OBJ_TOKEN_TYPE_object_signifier)
								{
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
									else if(OBJRequireToken(tokenizer, "s", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}
									else if(OBJRequireToken(tokenizer, "g", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}
									else if(OBJRequireToken(tokenizer, "usemtl", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}
									else if(OBJRequireToken(tokenizer, "mtllib", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}
									else
									{
										// NOTE(rjf): Warning: Unexpected token... skip.
										if(WarningCallback)
										{
											OBJToken unexpected_token = OBJPeekToken(tokenizer);
											OBJParseWarning warning;
											memset(&warning, 0, sizeof(OBJParseWarning));
											char buf[] = "Unexpected token.             ";
											memcpy(&buf[18], unexpected_token.string, unexpected_token.string_length);

											warning.type = OBJ_PARSE_WARNING_TYPE_unexpected_token;
											warning.filename = filename;
											warning.line = tokenizer->line;
											warning.message = buf;

											WarningCallback(warning);
										}
										OBJNextToken(tokenizer);
									}
								}
								else
								{
									break;
								}
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
								printf("ERROR: Out of memory.\n");
								goto end_parse;
							}
						}

						float *vertex_positions = (float *)memory;
						float *vertex_uvs = vertex_positions + num_positions * 3;
						float *vertex_normals = vertex_uvs + num_uvs * 2;
						int *face_vertices = (int *)(vertex_normals + num_normals*3);
						num_positions = 0;
						num_uvs = 0;
						num_normals = 0;
						num_face_vertices = 0;

						// NOTE(rjf): Reset tokenizer to the beginning of this object.
						{
							tokenizer->at = object_start;
						}

						// NOTE(rjf): Read in the OBJ vertex information.
						{
							for(;;)
							{
								OBJToken token = OBJPeekToken(tokenizer);
								if(token.type != OBJ_TOKEN_TYPE_null &&
								        token.type != OBJ_TOKEN_TYPE_object_signifier)
								{

									// NOTE(rjf): Vertex position
									if(OBJRequireToken(tokenizer, "v", 0))
									{
										OBJToken p1;
										OBJToken p2;
										OBJToken p3;
										if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &p1) &&
										        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &p2) &&
										        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &p3))
										{
											vertex_positions[num_positions++] = OBJTokenToFloat(p1);
											vertex_positions[num_positions++] = OBJTokenToFloat(p2);
											vertex_positions[num_positions++] = OBJTokenToFloat(p3);
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
											vertex_normals[num_normals++] = OBJTokenToFloat(n1);
											vertex_normals[num_normals++] = OBJTokenToFloat(n2);
											vertex_normals[num_normals++] = OBJTokenToFloat(n3);
										}
									}

									// NOTE(rjf): Vertex UV
									else if(OBJRequireToken(tokenizer, "vt", 0))
									{
										OBJToken u;
										OBJToken v;
										if(OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &u) &&
										        OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &v))
										{
											vertex_uvs[num_uvs++] = OBJTokenToFloat(u);
											vertex_uvs[num_uvs++] = OBJTokenToFloat(v);
											OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, 0);
										}
									}

									// NOTE(rjf): Face
									else if(OBJRequireToken(tokenizer, "f", 0))
									{
										int face_vertex_count = 0;
										int *this_face_vertices_ptr = face_vertices + num_face_vertices*3;
										unsigned int last_num_face_vertices = num_face_vertices;

										for(;;)
										{
											OBJToken position;
											OBJToken uv;
											OBJToken normal;
											memset(&position, 0, sizeof(OBJToken));
											memset(&uv, 0, sizeof(OBJToken));
											memset(&normal, 0, sizeof(OBJToken));


											if(OBJPeekToken(tokenizer).type == OBJ_TOKEN_TYPE_number)
											{
												OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &position);
												OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_face_index_divider, 0);
												OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &uv);
												OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_face_index_divider, 0);
												OBJRequireTokenType(tokenizer, OBJ_TOKEN_TYPE_number, &normal);

												face_vertices[num_face_vertices*3 + 0] = OBJTokenToInt(position) - vertex_position_index_offset;
												face_vertices[num_face_vertices*3 + 1] = OBJTokenToInt(uv) - vertex_uv_index_offset;
												face_vertices[num_face_vertices*3 + 2] = OBJTokenToInt(normal) - vertex_normal_index_offset;

												++num_face_vertices;
												++face_vertex_count;
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

											num_face_vertices = last_num_face_vertices;
											int this_face_vertices[256*3];

											for(int i = 0; i < face_vertex_count; ++i)
											{
												this_face_vertices[i*3 + 0] = this_face_vertices_ptr[i*3 + 0];
												this_face_vertices[i*3 + 1] = this_face_vertices_ptr[i*3 + 1];
												this_face_vertices[i*3 + 2] = this_face_vertices_ptr[i*3 + 2];
											}

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

												face_vertices[num_face_vertices*3 + 0] = triangle_p1[0];
												face_vertices[num_face_vertices*3 + 1] = triangle_p1[1];
												face_vertices[num_face_vertices*3 + 2] = triangle_p1[2];
												++num_face_vertices;

												face_vertices[num_face_vertices*3 + 0] = triangle_p2[0];
												face_vertices[num_face_vertices*3 + 1] = triangle_p2[1];
												face_vertices[num_face_vertices*3 + 2] = triangle_p2[2];
												++num_face_vertices;

												face_vertices[num_face_vertices*3 + 0] = triangle_p3[0];
												face_vertices[num_face_vertices*3 + 1] = triangle_p3[1];
												face_vertices[num_face_vertices*3 + 2] = triangle_p3[2];
												++num_face_vertices;
											}
										}
									}

									// NOTE(rjf): Shading hint (ignore)
									else if(OBJRequireToken(tokenizer, "s", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}

									// NOTE(rjf): Group label
									else if(OBJRequireToken(tokenizer, "g", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}

									// NOTE(rjf): Material Library
									else if(OBJRequireToken(tokenizer, "mtllib", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}

									// NOTE(rjf): Material Usage
									else if(OBJRequireToken(tokenizer, "usemtl", 0))
									{
										OBJSkipUntilNextLine(tokenizer);
									}

									else
									{
										// NOTE(rjf): Warning: Unexpected token... skip.
										if(WarningCallback)
										{
											OBJToken unexpected_token = OBJPeekToken(tokenizer);
											OBJParseWarning warning;
											memset(&warning, 0, sizeof(OBJParseWarning));
											char buf[] = "Unexpected token.            ";
											memcpy(&buf[18], unexpected_token.string, unexpected_token.string_length);

											warning.type = OBJ_PARSE_WARNING_TYPE_unexpected_token;
											warning.filename = filename;
											warning.line = tokenizer->line;
											warning.message = buf;

											WarningCallback(warning);
										}
										OBJNextToken(tokenizer);
									}
								}
								else
								{
									break;
								}
							}
						}

						// NOTE(rjf): OBJs store indices for positions, normals, and UVs, but OpenGL and
						// other APIs only allow us to have a single index buffer. So, when we get cases
						// like 2/3/4 and 2/3/5, we need to duplicate vertex information. We'll do this
						// in a vertex duplication step, where we'll keep track of position/UV/normal triplets,
						// and in cases where we find duplicates, we'll explicitly allocate storage for
						// duplicates, and fix up the face indices so that they point to the correct
						// indices.
						typedef struct VertexUVAndNormalIndices VertexUVAndNormalIndices;
						struct VertexUVAndNormalIndices
						{
							int position_index;
							int uv_index;
							int normal_index;
						};
						VertexUVAndNormalIndices *vertex_uv_and_normal_indices_buffer = 0;
						unsigned int num_face_vertices_with_duplicates = 0;
						{
							vertex_uv_and_normal_indices_buffer =
							    (VertexUVAndNormalIndices *)OBJParserArenaAllocate(arena, num_face_vertices * sizeof(VertexUVAndNormalIndices));

							if(!vertex_uv_and_normal_indices_buffer)
							{
								printf("ERROR: Out of memory.\n");
								goto end_parse;
							}

							for(unsigned int i = 0; i < num_face_vertices; ++i)
							{
								vertex_uv_and_normal_indices_buffer[i].position_index = 0;
								vertex_uv_and_normal_indices_buffer[i].uv_index = 0;
								vertex_uv_and_normal_indices_buffer[i].normal_index = 0;
							}

							num_face_vertices_with_duplicates = num_face_vertices;

							for(unsigned int i = 0; i < num_face_vertices; ++i)
							{
								// NOTE(rjf): We subtract 1 because the OBJ spec has 1-based indices.
								int position_index  = face_vertices[i*3 + 0] - 1;
								int uv_index        = face_vertices[i*3 + 1] - 1;
								int normal_index    = face_vertices[i*3 + 2] - 1;

								// NOTE(rjf): We've already written to this index, which means this vertex is a duplicate.
								if(vertex_uv_and_normal_indices_buffer[position_index].position_index ||
								        vertex_uv_and_normal_indices_buffer[position_index].uv_index ||
								        vertex_uv_and_normal_indices_buffer[position_index].normal_index)
								{
									if(vertex_uv_and_normal_indices_buffer[position_index].position_index != position_index+1 ||
									        vertex_uv_and_normal_indices_buffer[position_index].uv_index != uv_index+1 ||
									        vertex_uv_and_normal_indices_buffer[position_index].normal_index != normal_index+1)
									{
										// NOTE(rjf): This is a duplicate position, but it has different UV's or normals, so we
										// need to duplicate this vertex.
										{
											VertexUVAndNormalIndices *duplicate = (VertexUVAndNormalIndices *)OBJParserArenaAllocate(arena, sizeof(VertexUVAndNormalIndices));
											if(!duplicate)
											{
												printf("ERROR: Out of memory.\n");
												goto end_parse;
											}
											duplicate->position_index  = position_index+1;
											duplicate->uv_index        = uv_index+1;
											duplicate->normal_index    = normal_index+1;
										}

										// NOTE(rjf): Fix up the reference to this position.
										{
											face_vertices[i*3 + 0] = num_face_vertices_with_duplicates + 1;
										}

										++num_face_vertices_with_duplicates;
									}
								}
								else
								{
									// NOTE(rjf): We add 1 because the OBJ spec has 1-based indices, and we are checking
									// for 0's.
									vertex_uv_and_normal_indices_buffer[position_index].position_index  = position_index+1;
									vertex_uv_and_normal_indices_buffer[position_index].uv_index        = uv_index+1;
									vertex_uv_and_normal_indices_buffer[position_index].normal_index    = normal_index+1;
								}
							}
						}

						// NOTE(rjf): Now that we've duplicated vertices where necessary, we can create
						// final vertex and index buffers, where everything is correct for rendering.
						int final_vertex_buffer_write_number = 0;
						unsigned int bytes_needed_for_final_vertex_buffer = sizeof(float) * 8 * num_face_vertices_with_duplicates;
						float *final_vertex_buffer = (float *)OBJParserArenaAllocate(arena, bytes_needed_for_final_vertex_buffer);
						int final_index_buffer_write_number = 0;
						unsigned int bytes_needed_for_final_index_buffer = sizeof(int) * num_face_vertices_with_duplicates;
						int *final_index_buffer = (int *)OBJParserArenaAllocate(arena, bytes_needed_for_final_index_buffer);
						if(!final_vertex_buffer || !final_index_buffer)
						{
							printf("ERROR: Out of memory.\n");
							goto end_parse;
						}

						for(unsigned int i = 0; i < num_face_vertices; ++i)
						{
							int position_index = face_vertices[i*3 + 0] - 1;
							VertexUVAndNormalIndices *vertex_data = vertex_uv_and_normal_indices_buffer + position_index;

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

							final_vertex_buffer[(vertex_data->position_index-1)*8+0] = position[0];
							final_vertex_buffer[(vertex_data->position_index-1)*8+1] = position[1];
							final_vertex_buffer[(vertex_data->position_index-1)*8+2] = position[2];
							final_vertex_buffer[(vertex_data->position_index-1)*8+3] = uv[0];
							final_vertex_buffer[(vertex_data->position_index-1)*8+4] = uv[1];
							final_vertex_buffer[(vertex_data->position_index-1)*8+5] = normal[0];
							final_vertex_buffer[(vertex_data->position_index-1)*8+6] = normal[1];
							final_vertex_buffer[(vertex_data->position_index-1)*8+7] = normal[2];
							final_index_buffer[final_index_buffer_write_number++] = vertex_data->position_index-1;

							final_vertex_buffer_write_number += 8;
						}

						OBJParsedSubModelListNode *sub_model = (OBJParsedSubModelListNode *)
						                                       OBJParserArenaAllocate(arena, sizeof(OBJParsedSubModelListNode));

						if(!sub_model)
						{
							printf("ERROR: Out of memory.\n");
							goto end_parse;
						}

						// TODO(rjf): ASSUMES TRIANGLES!!!
						sub_model->flags = 0;
						sub_model->vertex_count = final_vertex_buffer_write_number / 8;
						sub_model->vertices = final_vertex_buffer;
						sub_model->index_count = final_index_buffer_write_number;
						sub_model->indices = final_index_buffer;
						sub_model->next = 0;
						*target_sub_model_list_node = sub_model;
						target_sub_model_list_node = &(*target_sub_model_list_node)->next;
						++sub_model_list_node_count;

						// NOTE(rjf): Update the offsets for this object, to correct for incorrect indices for
						// any future objects in this OBJ file.
						vertex_position_index_offset   += (int)num_positions / 3;
						vertex_uv_index_offset         += (int)num_uvs / 2;
						vertex_normal_index_offset     += (int)num_normals / 3;
					}

					else
					{
						break;
					}

				}

				// NOTE(rjf): After all polygon groups have been parsed, we want to convert
				// the linked list we've formed into a contiguous array that the user can
				// easily access.
				{
					model->sub_model_count = 0;
					model->sub_models = (ParsedOBJSubModel *)OBJParserArenaAllocate(arena, sizeof(ParsedOBJSubModel)*sub_model_list_node_count);
					if(!model->sub_models)
					{
						printf("ERROR: Out of memory.\n");
						goto end_parse;
					}

					for(OBJParsedSubModelListNode *node = model->first_sub_model;
					        node; node = node->next)
					{
						model->sub_models[model->sub_model_count].flags        = node->flags;
						model->sub_models[model->sub_model_count].vertex_count = node->vertex_count;
						model->sub_models[model->sub_model_count].vertices     = node->vertices;
						model->sub_models[model->sub_model_count].index_count  = node->index_count;
						model->sub_models[model->sub_model_count].indices      = node->indices;
						++model->sub_model_count;
					}
				}

			}

		}

	}

	obj->models = (ParsedOBJModel *)OBJParserArenaAllocate(arena, sizeof(ParsedOBJModel)*model_list_node_count);
	if(!obj->models)
	{
		printf("ERROR: Out of memory.\n");
		goto end_parse;
	}
	obj->model_count = 0;

	for(OBJParsedModelListNode *node = first_model_list_node;
	        node; node = node->next)
	{
		obj->models[obj->model_count].sub_model_count  = node->sub_model_count;
		obj->models[obj->model_count].sub_models       = node->sub_models;
		++obj->model_count;
	}

end_parse:
	;

	return obj_;
}

#endif // OBJ_PARSE_IMPLEMENTATION
