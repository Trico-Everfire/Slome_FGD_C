#pragma once

#include "tokenizer.h"

#include <stdbool.h>
#include <stddef.h>

#define MAX_STR_CHUNK_LENGTH 1024

#define AssignOrResizeArray( array, ArrayType, size )                  \
	if ( array )                                                       \
	{                                                                  \
		size++;                                                        \
		array = realloc( array, (int)( sizeof( ArrayType ) * size ) ); \
	}                                                                  \
	else                                                               \
	{                                                                  \
		array = malloc( sizeof( ArrayType ) );                         \
		size++;                                                        \
	}

enum ParseError
{
	NO_ERROR = 0,
	PARSE_ERROR
};

enum ClassType
{
	PointClass = 0,
	NPCClass,
	SolidClass,
	KeyFrameClass,
	MoveClass,
	BaseClass,
	FilterClass
};

enum SpecialType
{
	mapsize = 0,
	include,
	MaterialExclusion,
	AutoVisGroup
};

typedef struct vec2
{
	int x;
	int y;
} vec2_t;

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

// void, integer, float, string, bool

typedef enum EntityIOPropertyType
{
	t_string = 0,
	t_integer,
	t_float,
	t_bool,
	t_void,
	t_invalid,

} EntityIOPropertyType_t;

// typedef enum EntityPropertyType
//{
//	STRING = 0,
//	INTEGER,
//	FLOAT,
//	BOOLEAN,
//	FLAGS,
//	CHOICES
// } EntityPropertyType_t;
typedef struct choice
{
	char *value;
	char *displayName;
} choice_t;

typedef struct flags
{
	int value;
	bool checked;
	char *displayName;
} flags_t;

typedef struct EntityProperties
{
	char propertyName[32]; // We actually have a max for this :D 31 + \0
	char *type;
	char *displayName;	   // The following 3 are optional and may be empty as a result.
	char *defaultValue;
	char *entityDescription;
	bool readOnly;
	bool reportable;

	int choiceCount; // This is a special case if the EntityPropertyType is t_choices
	struct choice **choices;

	int flagCount; // This is a special case if the EntityPropertyType is t_flags
	struct flags **flags;

} EntityProperties_t;

typedef enum IO
{
	INPUT = 0,
	OUTPUT,
} InOut_e;

typedef struct InputOutput
{
	InOut_e putType;
	EntityIOPropertyType_t type;
	char *description;
	char *name;

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
	int entityCount;
	struct Entity **entities;
	struct vec2 mapSize;
	int materialExcludeCount;
	char **materialExclusions;
	int visGroupCount;
	int includeCount;
	char **includes;
	struct AutoVIsGroup **autoVisGroups;
} FGDFile_t;

struct FGDFile *parseFGDFile( char *file, size_t fileLength, enum ParseError *err );

char *getNextString( char *file, int *amount );

bool EndsWith( const char *str, const char *suffix );

bool processFGDStrings( TokenBlock_t **block, char **str );

void freeFGDFile( struct FGDFile *file );