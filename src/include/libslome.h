#pragma once

#include "../tokenizer.h"

#include <stdbool.h>
#include <stddef.h>
#include "enums.h"

#define SLOME_MAX_STR_CHUNK_LENGTH 1024

typedef struct Vec2
{
	int x;
	int y;
} Vec2_t;

typedef struct ParsingError
{
	enum ParseError err;
	int line;
	Vec2_t span;

}ParsingError_t;

typedef struct ClassProperty
{
	int propertyCount;
	char **properties;

} ClassProperty_t;

typedef struct ClassProperties
{
	char *name;
	int classPropertyCount;
	ClassProperty_t **classProperties;
} ClassProperties_t;

typedef enum EntityIOPropertyType
{
	t_string = 0,
	t_integer,
	t_float,
	t_bool,
	t_void,
	t_script,
	t_vector,
	t_target_destination,
	t_color255,
	t_custom,

} EntityIOPropertyType_t;

typedef struct Choice
{
	char *value;
	char *displayName;
} Choice_t;

typedef struct Flag
{
	int value;
	bool checked;
	char *displayName;
} Flag_t;

typedef struct EntityProperties
{
	char propertyName[32]; // We actually have a max for this :D 31 + \0
	char *type;
	char *displayName;	   // The following 3 are optional and may be empty as a result.
	char *defaultValue;
	char *propertyDescription;
	bool readOnly;
	bool reportable;

	int choiceCount; // This is a special case if the EntityPropertyType is t_choices
	Choice_t **choices;

	int flagCount; // This is a special case if the EntityPropertyType is t_flags
	Flag_t **flags;

} EntityProperties_t;

typedef enum IO
{
	INPUT = 0,
	OUTPUT,
} InOut_e;

typedef struct InputOutput
{
	char *name;
	char *description;
	InOut_e putType;
	char *stringType;
	EntityIOPropertyType_t type;

} InputOutput_t;

typedef struct Entity
{
	char *type;
	int classPropertyCount;
	struct ClassProperties **classProperties;
	char *entityName;
	char *entityDescription;
	int entityPropertyCount;
	struct EntityProperties **entityProperties;
	int IOCount;
	struct InputOutput **inputOutput;

} Entity_t;

typedef struct AutoVisGroupChild
{
	char *name;
	int childCount;
	char **children;
} AutoVisGroupChild_t;

typedef struct AutoVIsGroup
{
	char *name;
	int childCount;
	struct AutoVisGroupChild **children;
} AutoVIsGroup_t;

typedef struct FGDFile
{
	Vec2_t mapSize;
	int entityCount;
	struct Entity **entities;
	int materialExcludeCount;
	char **materialExclusions;
	int includeCount;
	char **includes;
	int visGroupCount;
	struct AutoVIsGroup **autoVisGroups;
} FGDFile_t;

struct FGDFile *ParseFGDFile( char *file, size_t fileLength, ParsingError_t *err );

void FreeFGDFile( struct FGDFile *file );