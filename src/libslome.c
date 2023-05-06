#include "include/libslome.h"

#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FirstOrNext( toNext, toNext2 ) toNext ? toNext->next : toNext2->first

#define Forward( block, failureResult )     \
	block = block->next;                    \
	if ( !block )                           \
		failureResult;                      \
	while ( block->token->type == COMMENT ) \
	{                                       \
		block = block->next;                \
		if ( !block )                       \
			failureResult;                  \
	}

#define AssignOrResizeArray( array, ArrayType, size, failureResult )             \
	{                                                                            \
		size++;                                                                  \
		ArrayType *temp = realloc( array, (int)( sizeof( ArrayType ) * size ) ); \
                                                                                 \
		if ( temp )                                                              \
		{                                                                        \
			array = temp;                                                        \
		}                                                                        \
		else                                                                     \
		{                                                                        \
			failureResult                                                        \
		}                                                                        \
	}

bool ProcessFGDStrings( TokenBlock_t **block, char **str )
{
#ifdef SLOME_UNIFIED_FGD
	char **combString = NULL;
	int totalLength = 0, combLength = 0;
	while ( ( *block )->token->type == STRING )
	{
		int len = strlen( ( *block )->token->string );
		AssignOrResizeArray( combString, char *, combLength, { return false; } );

		combString[combLength - 1] = strndup( ( *block )->token->string, len );

		totalLength += len;

		Forward( ( *block ), { return false; } );
		if ( ( *block )->token->type == PLUS )
		{
			Forward( ( *block ), { return false; } );
		}
	}

	char *descString = *str = malloc( totalLength + 1 );

	for ( int i = 0; i < combLength; i++ )
	{
		if ( i == 0 )
			strcpy( descString, combString[i] );
		else
			strcat( descString, combString[i] );
		free(combString[i]);
	}
	free(combString);
	descString[totalLength + 1] = '\0';
#else
	char *fields[50][SLOME_MAX_STR_CHUNK_LENGTH];

	int index = 0;
	while ( ( *block )->token->type == STRING )
	{
		int len = strlen( ( *block )->token->string );

		if ( len > SLOME_MAX_STR_CHUNK_LENGTH )
			return false;

		memset( fields[index], 0, SLOME_MAX_STR_CHUNK_LENGTH );

		strncpy( (char *)fields[index], ( *block )->token->string, SLOME_MAX_STR_CHUNK_LENGTH );

		index++;
		Forward( ( *block ), { return false; } );
		if ( ( *block )->token->type == PLUS )
		{
			Forward( ( *block ), { return false; } );
		}
	}

	char *descString = *str = malloc( index * SLOME_MAX_STR_CHUNK_LENGTH );
	memset( fields[index], 0, index * SLOME_MAX_STR_CHUNK_LENGTH );

	for ( int d = 0; d < index; d++ )
	{
		if ( d == 0 )
			strncpy( descString, fields[d], SLOME_MAX_STR_CHUNK_LENGTH );
		else
			strncat( descString, fields[d], SLOME_MAX_STR_CHUNK_LENGTH );
	}
#endif

	return true;
}

bool EndsWith( const char *str, const char *suffix )
{
	if ( !str || !suffix )
		return false;
	size_t lenstr = strlen( str );
	size_t lensuffix = strlen( suffix );
	if ( lensuffix > lenstr )
		return false;
	return strncmp( str + lenstr - lensuffix, suffix, lensuffix ) == 0;
}

TokenBlock_t *GetNext( TokenBlock_t *block )
{
	block = block->next;

	if ( !block )
		return NULL;
	while ( block->token->type == COMMENT )
		block = block->next;

	return block;
}

ParsingError_t ErrorFromBlock( TokenBlock_t *block, int lastLine )
{
	if ( !block )
	{
		ParsingError_t err = { PREMATURE_EOF, lastLine, { 0, 0 } };
		return err;
	}
	else
	{
		ParsingError_t err = { block->token->associatedError, block->token->line, { block->token->range.start, block->token->range.end } };
		return err;
	}
}

ParsingError_t ErrorFromValues( enum ParseError parseErr, int line, int start, int end )
{
	ParsingError_t err = { parseErr, line, { start, end } };
	return err;
}

#ifdef SLOME_UNIFIED_FGD
bool TagListDelimiter( TokenBlock_t **block, TagList_t *tagList )
{
	// if there are more than 40 non comma separated parameters, you're doing something wrong.
	// The value is already so high in case anyone adds new fgd class parameters in the future that require them.
	char *fields[40];
	memset( fields, 0x0, sizeof( char * ) * 40 );

	int i = 0;
	bool hasPlus = false;
	while ( ( *block )->token->type == LITERAL || ( *block )->token->type == PLUS || ( *block )->token->type == COMMA || ( *block )->token->type == STRING || ( *block )->token->type == NUMBER )
	{
		if ( i > 40 ) // wtf happened?
		{
			return false;
		}

		if ( ( *block )->token->type == PLUS )
		{
			hasPlus = true;
			Forward( ( *block ), return false );
			continue;
		}

		if ( ( *block )->token->type == COMMA )
		{
			tagList->tags = malloc( sizeof( char * ) * i );
			tagList->tagCount = i;
			for ( int j = 0; j < i; j++ )
			{
				tagList->tags[j] = fields[j];
			}

			i = 0;
			Forward( ( *block ), return false );
			continue;
		}

		if ( !hasPlus )
		{
			fields[i] = strdup( ( *block )->token->string );
			i++;
		}
		else
		{
			uint length = strlen( ( *block )->token->string ) + 1;
			char *str = calloc( '\0', length );

			strncat( str, "+", 1 );
			strncat( str, ( *block )->token->string, length - 1 );
			fields[i] = str;
			i++;
		}

		Forward( ( *block ), return false );
	}

	if ( i > 0 )
	{
		tagList->tags = malloc( sizeof( char * ) * i );
		tagList->tagCount = i;
		for ( int j = 0; j < i; j++ )
		{
			tagList->tags[j] = fields[j];
		}
	}

	if ( ( *block )->token->type != CLOSE_BRACKET )
		return false;

	return true;
}
#endif

FGDFile_t *ParseFGDFile( char *file, size_t fileLength, ParsingError_t *err )
{
	*err = ErrorFromValues( NO_ERROR, 0, 0, 0 );

	FGDFile_t *fgdFile = malloc( sizeof( FGDFile_t ) );

	memset( fgdFile, 0, sizeof( FGDFile_t ) );

	fgdFile->entityCount = 0;
	fgdFile->visGroupCount = 0;
	fgdFile->materialExcludeCount = 0;

	Tokenizer_t *tokenizer = GetNewTokenList();

	char *ownedChar = malloc( fileLength );
	memcpy( ownedChar, file, fileLength );

	if ( !TokenizeFile( ownedChar, fileLength, &tokenizer ) || !tokenizer->first ) // We have no tokens.
	{
		free( ownedChar );
		FreeTokenizer( tokenizer );
		*err = ErrorFromValues( TOKENIZATION_ERROR, fileLength, 0, fileLength );
		return NULL;
	}

	char *typeStrings[9] = { "string", "integer", "float", "bool", "void", "script", "vector", "target_destination", "color255" };
	EntityIOPropertyType_t typeList[9] = { t_string, t_integer, t_float, t_bool, t_void, t_script, t_vector, t_target_destination, t_color255 };

	int lastLineInFile = tokenizer->next->token->line;

	TokenBlock_t *block = NULL;
	while ( ( block = FirstOrNext( block, tokenizer ) ) )
	{
		if ( block->token->type != DEFINITION )
			continue;

		if ( strcasecmp( block->token->string, "@mapsize" ) == 0 )
		{
			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != OPEN_PARENTHESIS )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != NUMBER )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			fgdFile->mapSize.x = atoi( block->token->string );

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != COMMA )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != NUMBER )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			fgdFile->mapSize.y = atoi( block->token->string );

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != CLOSE_PARENTHESIS )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			continue;
		}

		if ( strcasecmp( block->token->string, "@AutoVisgroup" ) == 0 )
		{
			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != EQUALS )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != STRING )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			AssignOrResizeArray( fgdFile->autoVisGroups, AutoVIsGroup_t *, fgdFile->visGroupCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );

			AutoVIsGroup_t *visGroup = fgdFile->autoVisGroups[fgdFile->visGroupCount - 1] = malloc( sizeof( AutoVIsGroup_t ) );
			memset( visGroup, 0, sizeof( AutoVIsGroup_t ) );

			visGroup->name = strdup( block->token->string );

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != OPEN_BRACKET )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != STRING && block->token->type != CLOSE_BRACKET )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			while ( block->token->type == STRING )
			{
				AssignOrResizeArray( visGroup->children, AutoVisGroupChild_t *, visGroup->childCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
				AutoVisGroupChild_t *visGroupChild = visGroup->children[visGroup->childCount - 1] = malloc( sizeof( AutoVisGroupChild_t ) );
				memset( visGroupChild, 0, sizeof( AutoVisGroupChild_t ) );

				visGroupChild->name = strdup( block->token->string );

				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
				if ( block->token->type != OPEN_BRACKET )
				{
					*err = ErrorFromBlock( block, lastLineInFile );
					goto onError;
				}

				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
				if ( block->token->type != STRING && block->token->type != CLOSE_BRACKET )
				{
					*err = ErrorFromBlock( block, lastLineInFile );
					goto onError;
				}

				while ( block->token->type == STRING )
				{
					if ( block->token->type != STRING )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					AssignOrResizeArray( visGroupChild->children, char *, visGroupChild->childCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
					visGroupChild->children[visGroupChild->childCount - 1] = strdup( block->token->string );

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
				}

				if ( block->token->type != CLOSE_BRACKET )
				{
					*err = ErrorFromBlock( block, lastLineInFile );
					goto onError;
				}

				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			}

			if ( block->token->type != CLOSE_BRACKET )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			continue;
		}

		if ( strcasecmp( block->token->string, "@include" ) == 0 )
		{
			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

			if ( block->token->type != STRING )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			AssignOrResizeArray( fgdFile->includes, char *, fgdFile->includeCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
			fgdFile->includes[fgdFile->includeCount - 1] = strdup( block->token->string );
			continue;
		}

		if ( strcasecmp( block->token->string, "@MaterialExclusion" ) == 0 )
		{
			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

			if ( block->token->type != OPEN_BRACKET )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

			while ( block->token->type == STRING )
			{
				AssignOrResizeArray( fgdFile->materialExclusions, char *, fgdFile->materialExcludeCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
				fgdFile->materialExclusions[fgdFile->materialExcludeCount - 1] = strdup( block->token->string );

				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			}

			if ( block->token->type != CLOSE_BRACKET )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			continue;
		}

		if ( EndsWith( block->token->string, "Class" ) )
		{
			AssignOrResizeArray( fgdFile->entities, Entity_t *, fgdFile->entityCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
			Entity_t *entity = fgdFile->entities[fgdFile->entityCount - 1] = malloc( sizeof( Entity_t ) );
			memset( entity, 0x0, sizeof( Entity_t ) );
			entity->classPropertyCount = 0;
			entity->entityPropertyCount = 0;
			entity->IOCount = 0;
#ifdef SLOME_UNIFIED_FGD
			entity->resourceCount = 0;
			entity->resources = NULL;
#endif

			entity->type = strdup( block->token->string );

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

			while ( block->token->type == LITERAL )
			{
				AssignOrResizeArray( entity->classProperties, ClassProperties_t *, entity->classPropertyCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
				ClassProperties_t *classProperties = entity->classProperties[entity->classPropertyCount - 1] = malloc( sizeof( ClassProperties_t ) );

				classProperties->name = strdup( block->token->string );
				classProperties->classPropertyCount = 0;
				classProperties->classProperties = NULL;

				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
				if ( block->token->type == OPEN_PARENTHESIS )
				{
					// if there are more than 40 non comma separated parameters, you're doing something wrong.
					// The value is already so high in case anyone adds new fgd class parameters in the future that require them.
					char *fields[40];
					memset( fields, 0x0, sizeof( char * ) * 40 );

					int i = 0;

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					while ( block->token->type == LITERAL || block->token->type == COMMA || block->token->type == STRING || block->token->type == NUMBER )
					{
						if ( i > 40 ) // wtf happened?
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}

						if ( block->token->type == COMMA )
						{
							AssignOrResizeArray( classProperties->classProperties, ClassProperty_t *, classProperties->classPropertyCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );

							ClassProperty_t *property = classProperties->classProperties[classProperties->classPropertyCount - 1] = malloc( sizeof( ClassProperty_t ) );
							property->propertyCount = i;
							property->properties = malloc( sizeof( char * ) * i );
							for ( int j = 0; j < i; j++ )
							{
								property->properties[j] = fields[j];
							}

							i = 0;
							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
							continue;
						}

						fields[i] = strdup( block->token->string );
						i++;

						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					}

					if ( i > 0 )
					{
						AssignOrResizeArray( classProperties->classProperties, ClassProperty_t *, classProperties->classPropertyCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
						ClassProperty_t *property = classProperties->classProperties[classProperties->classPropertyCount - 1] = malloc( sizeof( ClassProperty_t ) );
						property->propertyCount = i;
						property->properties = malloc( sizeof( char * ) * i );
						for ( int j = 0; j < i; j++ )
						{
							property->properties[j] = fields[j];
						}

						i = 0;
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
						continue;
					}

					if ( block->token->type != CLOSE_PARENTHESIS )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
				}
			}

			if ( block->token->type != EQUALS )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			if ( block->token->type != LITERAL )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			entity->entityName = strdup( block->token->string );

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

			if ( block->token->type == COLUMN )
			{
				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

				if ( block->token->type != STRING )
				{
					*err = ErrorFromBlock( block, lastLineInFile );
					goto onError;
				}

				if ( !ProcessFGDStrings( &block, &entity->entityDescription ) )
				{
					*err = ErrorFromBlock( block, lastLineInFile );
					goto onError;
				}
			}

			if ( block->token->type != OPEN_BRACKET )
			{
				*err = ErrorFromBlock( block, lastLineInFile );
				goto onError;
			}

			Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			while ( block->token->type != CLOSE_BRACKET )
			{
#ifdef SLOME_UNIFIED_FGD
				if ( strcasecmp( block->token->string, "@resources" ) == 0 )
				{
					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != OPEN_BRACKET )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					while ( block->token->type != CLOSE_BRACKET )
					{
						if ( block->token->type != LITERAL )
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}

						AssignOrResizeArray( entity->resources, EntityResource_t *, entity->resourceCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
						EntityResource_t *resource = entity->resources[entity->resourceCount - 1] = malloc( sizeof( EntityResource_t ) );
						resource->key = NULL;
						resource->value = NULL;
						resource->tagList.tagCount = 0;
						resource->tagList.tags = NULL;

						resource->key = strdup( block->token->string );

						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

						resource->value = strdup( block->token->string );

						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

						if ( block->token->type == OPEN_BRACKET )
						{
							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
							if ( !TagListDelimiter( &block, &resource->tagList ) )
							{
								*err = ErrorFromBlock( block, lastLineInFile );
								goto onError;
							}
							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
						}
					}

					if ( block->token->type != CLOSE_BRACKET )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					continue;
				}
#endif

				if ( block->token->type != LITERAL )
				{
					*err = ErrorFromBlock( block, lastLineInFile );
					goto onError;
				}

				if ( strcasecmp( block->token->string, "input" ) == 0 || strcasecmp( block->token->string, "output" ) == 0 )
				{
					AssignOrResizeArray( entity->inputOutput, InputOutput_t *, entity->IOCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
					InputOutput_t *inputOutput = entity->inputOutput[entity->IOCount - 1] = malloc( sizeof( InputOutput_t ) );
					inputOutput->description = NULL;
					inputOutput->name = NULL;
#ifdef SLOME_UNIFIED_FGD
					inputOutput->tagList.tagCount = 0;
					inputOutput->tagList.tags = NULL;
#endif

					inputOutput->putType = strcasecmp( block->token->string, "input" ) == 0 ? INPUT : OUTPUT;

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					inputOutput->name = strdup( block->token->string );

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

#ifdef SLOME_UNIFIED_FGD
					if ( block->token->type == OPEN_BRACKET )
					{
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
						if ( !TagListDelimiter( &block, &inputOutput->tagList ) )
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					}
#endif

					if ( block->token->type != OPEN_PARENTHESIS )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != LITERAL )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					int index = 0;
					while ( index < 9 )
					{
						if ( strcasecmp( typeStrings[index], block->token->string ) == 0 )
							break;
						index++;
					}
					if ( index == 9 )
						inputOutput->type = t_custom;
					else
						inputOutput->type = typeList[index];

					inputOutput->stringType = strdup( block->token->string );

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != CLOSE_PARENTHESIS )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type == COLUMN )
					{
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

						if ( block->token->type != STRING )
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}

						if ( !ProcessFGDStrings( &block, &inputOutput->description ) )
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}
					}

					continue;
				}
				else
				{
					AssignOrResizeArray( entity->entityProperties, EntityProperties_t *, entity->entityPropertyCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
					EntityProperties_t *entityProperties = entity->entityProperties[entity->entityPropertyCount - 1] = malloc( sizeof( EntityProperties_t ) );
					memset( entityProperties, 0x0, sizeof( EntityProperties_t ) );
					entityProperties->flagCount = 0;
					entityProperties->choiceCount = 0;
#ifdef SLOME_UNIFIED_FGD
					entityProperties->tagList.tagCount = 0;
					entityProperties->tagList.tags = NULL;
#endif
					entityProperties->readOnly = false;
					entityProperties->reportable = false;

					entityProperties->propertyName[32] = '\0'; // last character should always be a null terminator.
					strncpy( entityProperties->propertyName, block->token->string, 31 );

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

#ifdef SLOME_UNIFIED_FGD
					if ( block->token->type == OPEN_BRACKET )
					{
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
						if ( !TagListDelimiter( &block, &entityProperties->tagList ) )
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					}
#endif

					if ( block->token->type != OPEN_PARENTHESIS )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					if ( block->token->type != LITERAL )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					entityProperties->type = strdup( block->token->string );

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					if ( block->token->type != CLOSE_PARENTHESIS )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( strcasecmp( block->token->string, "readonly" ) == 0 )
					{
						entityProperties->readOnly = true;
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					}

					if ( ( strcasecmp( block->token->string, "*" ) == 0 || strcasecmp( block->token->string, "report" ) == 0 ) )
					{
						entityProperties->reportable = true;
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					}

					if ( block->token->type == EQUALS )
					{
						goto isFOC;
					}

					if ( block->token->type != COLUMN )
						continue;

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != STRING )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					entityProperties->displayName = strdup( block->token->string );

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type == EQUALS )
					{
						goto isFOC;
					}

					if ( block->token->type != COLUMN )
						continue;

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != COLUMN )
					{
						entityProperties->defaultValue = strdup( block->token->string );
						Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
					}

					if ( block->token->type == EQUALS )
					{
						goto isFOC;
					}

					if ( block->token->type != COLUMN )
						continue;

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != STRING )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					if ( !ProcessFGDStrings( &block, &entityProperties->propertyDescription ) )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					if ( block->token->type == EQUALS )
					{
						goto isFOC;
					}

					continue;

				isFOC:
				{
					bool isFlags = strcasecmp( entityProperties->type, "flags" ) == 0;

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					if ( block->token->type != OPEN_BRACKET )
					{
						*err = ErrorFromBlock( block, lastLineInFile );
						goto onError;
					}

					Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

					while ( block->token->type != CLOSE_BRACKET )
					{
						if ( isFlags && block->token->type != NUMBER )
						{
							*err = ErrorFromBlock( block, lastLineInFile );
							goto onError;
						}

						if ( isFlags )
						{
							AssignOrResizeArray( entityProperties->flags, Flag_t *, entityProperties->flagCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
							Flag_t *flags = entityProperties->flags[entityProperties->flagCount - 1] = malloc( sizeof( Flag_t ) );
#ifdef SLOME_UNIFIED_FGD
							flags->tagList.tagCount = 0;
							flags->tagList.tags = NULL;
#endif
							flags->value = atoi( block->token->string );

							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
							if ( block->token->type != COLUMN )
							{
								*err = ErrorFromBlock( block, lastLineInFile );
								goto onError;
							}

							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
							if ( block->token->type != STRING )
							{
								*err = ErrorFromBlock( block, lastLineInFile );
								goto onError;
							}

							flags->displayName = strdup( block->token->string );

							if ( GetNext( block )->token->type == COLUMN )
							{
								Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

								Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
								if ( block->token->type != NUMBER )
								{
									*err = ErrorFromBlock( block, lastLineInFile );
									goto onError;
								}
								flags->checked = strcasecmp( block->token->string, "1" ) == 0;

#ifdef SLOME_UNIFIED_FGD
								if ( GetNext( block )->token->type == OPEN_BRACKET )
								{
									Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
									Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
									if ( !TagListDelimiter( &block, &flags->tagList ) )
									{
										*err = ErrorFromBlock( block, lastLineInFile );
										goto onError;
									}
								}
#endif
							}

							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
						}
						else
						{
							AssignOrResizeArray( entityProperties->choices, Choice_t *, entityProperties->choiceCount, { *err = ErrorFromValues( ALLOCATION_FAILURE, block->token->line, block->token->range.start, block->token->range.end ); goto onError; } );
							Choice_t *choice = entityProperties->choices[entityProperties->choiceCount - 1] = malloc( sizeof( Choice_t ) );
#ifdef SLOME_UNIFIED_FGD
							choice->tagList.tagCount = 0;
							choice->tagList.tags = NULL;
#endif
							choice->value = strdup( block->token->string );

							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
							if ( block->token->type != COLUMN )
							{
								*err = ErrorFromBlock( block, lastLineInFile );
								goto onError;
							}

							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
							if ( block->token->type != STRING )
							{
								*err = ErrorFromBlock( block, lastLineInFile );
								goto onError;
							}

							choice->displayName = strdup( block->token->string );

#ifdef SLOME_UNIFIED_FGD
							if ( GetNext( block )->token->type == OPEN_BRACKET )
							{
								Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
								Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );

								if ( !TagListDelimiter( &block, &choice->tagList ) )
								{
									*err = ErrorFromBlock( block, lastLineInFile );
									goto onError;
								}
							}
#endif

							Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
						}
					}
				}
				}

				Forward( block, { *err = ErrorFromBlock( block, lastLineInFile ); goto onError; } );
			}
		}
	}

	free( ownedChar );
	FreeTokenizer( tokenizer );

	return fgdFile;

onError:
	FreeFGDFile( fgdFile );
	free( ownedChar );
	FreeTokenizer( tokenizer );
	return NULL;
}

void FreeFGDFile( struct FGDFile *file )
{
	if ( !file )
		return;

	for ( int i = 0; i < file->entityCount; i++ )
	{
		if ( file->entities[i]->type )
			free( file->entities[i]->type );

		if ( file->entities[i]->entityName )
			free( file->entities[i]->entityName );

		if ( file->entities[i]->entityDescription )
			free( file->entities[i]->entityDescription );
#ifdef SLOME_UNIFIED_FGD
		for ( int p = 0; p < file->entities[i]->resourceCount; p++ )
		{
			free( file->entities[i]->resources[p]->key );
			free( file->entities[i]->resources[p]->value );
			for ( int k = 0; k < file->entities[i]->resources[p]->tagList.tagCount; k++ )
				free( file->entities[i]->resources[p]->tagList.tags[k] );
			if ( file->entities[i]->resources[p]->tagList.tags )
				free( file->entities[i]->resources[p]->tagList.tags );
			free( file->entities[i]->resources[p] );
		}

		if ( file->entities[i]->resources )
			free( file->entities[i]->resources );
#endif

		for ( int p = 0; p < file->entities[i]->IOCount; p++ )
		{
			if ( file->entities[i]->inputOutput[p]->name )
				free( file->entities[i]->inputOutput[p]->name );

			if ( file->entities[i]->inputOutput[p]->description )
				free( file->entities[i]->inputOutput[p]->description );

			if ( file->entities[i]->inputOutput[p]->stringType )
				free( file->entities[i]->inputOutput[p]->stringType );
#ifdef SLOME_UNIFIED_FGD
			for ( int k = 0; k < file->entities[i]->inputOutput[p]->tagList.tagCount; k++ )
				free( file->entities[i]->inputOutput[p]->tagList.tags[k] );
			if ( file->entities[i]->inputOutput[p]->tagList.tags )
				free( file->entities[i]->inputOutput[p]->tagList.tags );
#endif

			if ( file->entities[i]->inputOutput[p] )
				free( file->entities[i]->inputOutput[p] );
		}

		if ( file->entities[i]->inputOutput )
			free( file->entities[i]->inputOutput );

		for ( int p = 0; p < file->entities[i]->entityPropertyCount; p++ )
		{
			if ( file->entities[i]->entityProperties[p]->propertyDescription )
				free( file->entities[i]->entityProperties[p]->propertyDescription );
			if ( file->entities[i]->entityProperties[p]->displayName )
				free( file->entities[i]->entityProperties[p]->displayName );
			if ( file->entities[i]->entityProperties[p]->defaultValue )
				free( file->entities[i]->entityProperties[p]->defaultValue );
			if ( file->entities[i]->entityProperties[p]->type )
				free( file->entities[i]->entityProperties[p]->type );
#ifdef SLOME_UNIFIED_FGD
			for ( int k = 0; k < file->entities[i]->entityProperties[p]->tagList.tagCount; k++ )
				free( file->entities[i]->entityProperties[p]->tagList.tags[k] );
			if ( file->entities[i]->entityProperties[p]->tagList.tags )
				free( file->entities[i]->entityProperties[p]->tagList.tags );
#endif

			if ( file->entities[i]->entityProperties[p]->choices )
			{
				for ( int j = 0; j < file->entities[i]->entityProperties[p]->choiceCount; j++ )
				{
					if ( file->entities[i]->entityProperties[p]->choices[j]->value )
						free( file->entities[i]->entityProperties[p]->choices[j]->value );
					if ( file->entities[i]->entityProperties[p]->choices[j]->displayName )
						free( file->entities[i]->entityProperties[p]->choices[j]->displayName );
#ifdef SLOME_UNIFIED_FGD
					for ( int k = 0; k < file->entities[i]->entityProperties[p]->choices[j]->tagList.tagCount; k++ )
						free( file->entities[i]->entityProperties[p]->choices[j]->tagList.tags[k] );
					if ( file->entities[i]->entityProperties[p]->choices[j]->tagList.tags )
						free( file->entities[i]->entityProperties[p]->choices[j]->tagList.tags );
#endif

					free( file->entities[i]->entityProperties[p]->choices[j] );
				}

				free( file->entities[i]->entityProperties[p]->choices );
			}

			if ( file->entities[i]->entityProperties[p]->flags )
			{
				for ( int j = 0; j < file->entities[i]->entityProperties[p]->flagCount; j++ )
				{
					if ( file->entities[i]->entityProperties[p]->flags[j]->displayName )
						free( file->entities[i]->entityProperties[p]->flags[j]->displayName );
#ifdef SLOME_UNIFIED_FGD
					for ( int k = 0; k < file->entities[i]->entityProperties[p]->flags[j]->tagList.tagCount; k++ )
						free( file->entities[i]->entityProperties[p]->flags[j]->tagList.tags[k] );
					if ( file->entities[i]->entityProperties[p]->flags[j]->tagList.tags )
						free( file->entities[i]->entityProperties[p]->flags[j]->tagList.tags );
#endif

					free( file->entities[i]->entityProperties[p]->flags[j] );
				}
				free( file->entities[i]->entityProperties[p]->flags );
			}

			if ( file->entities[i]->entityProperties[p] )
				free( file->entities[i]->entityProperties[p] );
		}

		if ( file->entities[i]->entityProperties )
			free( file->entities[i]->entityProperties );

		for ( int p = 0; p < file->entities[i]->classPropertyCount; p++ )
		{
			if ( file->entities[i]->classProperties[p]->name )
				free( file->entities[i]->classProperties[p]->name );
			if ( file->entities[i]->classProperties[p]->classProperties )
			{
				for ( int j = 0; j < file->entities[i]->classProperties[p]->classPropertyCount; j++ )
				{
					for ( int k = 0; k < file->entities[i]->classProperties[p]->classProperties[j]->propertyCount; k++ )
						free( file->entities[i]->classProperties[p]->classProperties[j]->properties[k] );
					free( file->entities[i]->classProperties[p]->classProperties[j]->properties );
					free( file->entities[i]->classProperties[p]->classProperties[j] );
				}

				free( file->entities[i]->classProperties[p]->classProperties );
			}
			if ( file->entities[i]->classProperties[p] )
				free( file->entities[i]->classProperties[p] );
		}

		if ( file->entities[i]->classProperties )
			free( file->entities[i]->classProperties );

		if ( file->entities[i] )
			free( file->entities[i] );
	}

	if ( file->entities )
		free( file->entities );

	for ( int i = 0; i < file->visGroupCount; i++ )
	{
		if ( file->autoVisGroups[i]->name )
			free( file->autoVisGroups[i]->name );

		for ( int j = 0; j < file->autoVisGroups[i]->childCount; j++ )
		{
			if ( file->autoVisGroups[i]->children[j]->name )
				free( file->autoVisGroups[i]->children[j]->name );

			for ( int k = 0; k < file->autoVisGroups[i]->children[j]->childCount; k++ )
			{
				if ( file->autoVisGroups[i]->children[j]->children[k] )
					free( file->autoVisGroups[i]->children[j]->children[k] );
			}

			free( file->autoVisGroups[i]->children[j]->children );
			free( file->autoVisGroups[i]->children[j] );
		}
		if ( file->autoVisGroups[i]->children )
			free( file->autoVisGroups[i]->children );
		free( file->autoVisGroups[i] );
	}

	if ( file->autoVisGroups )
		free( file->autoVisGroups );

	for ( int i = 0; i < file->materialExcludeCount; i++ )
	{
		if ( file->materialExclusions[i] )
			free( file->materialExclusions[i] );
	}

	if ( file->materialExclusions )
		free( file->materialExclusions );

	for ( int i = 0; i < file->includeCount; i++ )
	{
		if ( file->includes[i] )
			free( file->includes[i] );
	}

	if ( file->includes )
		free( file->includes );

	free( file );
}
