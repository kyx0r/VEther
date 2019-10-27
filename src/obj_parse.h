#ifndef OBJ_PARSE_H_
#define OBJ_PARSE_H_

#ifndef OBJ_PARSE_NO_CRT
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include "zone.h"

#define OBJ_PARSE_IMPLEMENTATION

#define PARSED_OBJ_MODEL_FLAG_POSITION   (1<<0)
#define PARSED_OBJ_MODEL_FLAG_UV         (1<<1)
#define PARSED_OBJ_MODEL_FLAG_NORMAL     (1<<2)

typedef struct ParsedOBJMaterial ParsedOBJMaterial;
struct ParsedOBJMaterial
{
	char *material_name;
};

typedef struct ParsedOBJSubModel ParsedOBJSubModel;
struct ParsedOBJSubModel
{
	ParsedOBJMaterial material;
	int flags;
	unsigned int vertex_count;
	float *vertices;
	unsigned int index_count;
	int *indices;
};

typedef struct ParsedOBJModel ParsedOBJModel;
struct ParsedOBJModel
{
	unsigned int sub_model_count;
	ParsedOBJSubModel *sub_models;
};

typedef struct ParsedOBJ ParsedOBJ;
struct ParsedOBJ
{
	char malloc;
	unsigned int model_count;
	ParsedOBJModel *models;
	void *parse_memory_to_free;
	unsigned int material_library_count;
	char **material_libraries;
};

typedef struct OBJParseInfo OBJParseInfo;
ParsedOBJ  LoadOBJ(char *filename);
ParsedOBJ  ParseOBJ(OBJParseInfo *parse_info);
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

#endif // OBJ_PARSE_H_
