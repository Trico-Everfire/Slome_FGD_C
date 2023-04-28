#include "library.h"

#include "tokenizer.h"

#include <ctype.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

FGDFile_t *parseFGDFile( char *file, size_t fileLength, enum ParseError *err )
{
	*err = NO_ERROR;

	FGDFile_t *fgdFile = malloc( sizeof( FGDFile_t ) );

	memset( fgdFile, 0, sizeof( FGDFile_t ) );

	fgdFile->entityCount = 0;
	fgdFile->visGroupCount = 0;
	fgdFile->materialExcludeCount = 0;

	Tokenizer_t *tokenizer = GetNewTokenList();

	char *ownedChar = malloc( fileLength );
	memcpy( ownedChar, file, fileLength );

	char *seditedFile = ownedChar;
	char *eof = seditedFile + fileLength;

	char singleTokens[] = "{}[](),:=+";
	TokenType_e valueTokens[] = { OPEN_BRACE, CLOSE_BRACE, OPEN_BRACKET, CLOSE_BRACKET, OPEN_PARENTHESIS, CLOSE_PARENTHESIS, COMMA, COLUMN, EQUALS, PLUS };

	char *typeStrings[9] = { "string", "integer", "float", "bool", "void", "script", "vector", "target_destination", "color255" };
	EntityIOPropertyType_t typeList[9] = { t_string, t_integer, t_float, t_bool, t_void, t_script,	t_vector, t_target_destination, t_color255 };

	for ( int i = 0, ln = 1; eof != seditedFile; i++, seditedFile++ )
	{
		char c = *seditedFile;

		if ( c == '\t' )
			continue;

		if ( c == '\r' )
			continue;

		if ( c == '\n' )
		{
			ln++;
			continue;
		}

		if ( c == '"' )
		{
			int currentln = ln;
			int currentLength = i;
			char *currentPosition = seditedFile;
			c = '\t'; // We can get away with this to trick the while loop :)
			while ( c != '"' )
			{
				seditedFile++;
				c = *seditedFile;
				i++;
				if ( c == '\n' )
					ln++;
			}
			seditedFile++;
			i++;
			Token_t *commentToken = generateEmptyToken();
			commentToken->line = currentln;
			commentToken->type = STRING;
			Range_t range = { currentLength, i };
			commentToken->range = range;
			commentToken->string = strndup( currentPosition, i - currentLength );
			PushToTokenList( tokenizer, commentToken );
			seditedFile--;
			i--;
			continue;
		}

		if ( c == '/' && *( seditedFile + 1 ) == '/' )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			while ( c != '\n' )
			{
				c = *seditedFile;
				i++;
				seditedFile++;
			}
			seditedFile--;
			i--;

			Token_t *commentToken = generateEmptyToken();
			commentToken->line = ln;
			commentToken->type = COMMENT;
			Range_t range = { currentLength, i };
			commentToken->range = range;
			commentToken->string = strndup( currentPosition, i - currentLength );
			PushToTokenList( tokenizer, commentToken );

			seditedFile--;
			i--;
			continue;
		}

		if ( c == '@' )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			while ( c != '\n' && c != '\t' && c != '\r' && c != ' ' && c != '(' )
			{
				c = *seditedFile;
				i++;
				seditedFile++;
			}
			seditedFile--;
			i--;

			if ( c == '\n' )
				ln++;
			Token_t *commentToken = generateEmptyToken();
			commentToken->line = ln;
			commentToken->type = DEFINITION;
			Range_t range = { currentLength, i };
			commentToken->range = range;
			commentToken->string = strndup( currentPosition, i - currentLength );
			PushToTokenList( tokenizer, commentToken );

			seditedFile--;
			i--;
			continue;
		}

		if ( isdigit( c ) != 0 || ( c == '-' && isdigit( *( seditedFile + 1 ) ) ) )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;

			if ( c == '-' )
			{
				seditedFile++;
				i++;
				c = *seditedFile;
			}

			while ( isdigit( c ) != 0 )
			{
				c = *seditedFile;
				i++;
				seditedFile++;
			}
			seditedFile--;
			i--;

			Token_t *commentToken = generateEmptyToken();
			commentToken->line = ln;
			commentToken->type = NUMBER;
			Range_t range = { currentLength, i };
			commentToken->range = range;
			commentToken->string = strndup( currentPosition, i - currentLength );
			PushToTokenList( tokenizer, commentToken );
			seditedFile--;
			i--;
			continue;
		}

		char *valueKey = strchr( singleTokens, c );

		if ( valueKey )
		{
			int spaces = (int)( (int)( (char *)valueKey - (char *)singleTokens ) / sizeof( char ) ); // char should be 1, but I am sanity checking it anyway.
			TokenType_e tType = valueTokens[spaces];
			Token_t *commentToken = generateEmptyToken();
			commentToken->line = ln;
			commentToken->type = tType;
			Range_t range = { i, i + 1 };
			commentToken->range = range;
			char temp[] = { c };
			commentToken->string = strndup( temp, 1 );
			PushToTokenList( tokenizer, commentToken );
			continue;
		}

		if ( c != ' ' )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			while ( c != '\n' && c != ' ' && c != '\t' && c != '\r' && !strchr( singleTokens, c ) )
			{
				seditedFile++;
				c = *seditedFile;
				i++;
			}

			Token_t *commentToken = generateEmptyToken();
			commentToken->line = ln;
			commentToken->type = LITERAL;
			Range_t range = { currentLength, i };
			commentToken->range = range;
			commentToken->string = strndup( currentPosition, i - currentLength );
			PushToTokenList( tokenizer, commentToken );
			seditedFile--;
			i--;
			continue;
		}
	}

	if ( tokenizer->first )
	{
		TokenBlock_t *block = NULL;

		while ( ( block = firstOrNext( block, tokenizer ) ) )
		{
			if ( block->token->type == DEFINITION )
			{
				if ( strcasecmp( block->token->string, "@mapsize" ) == 0 )
				{
					Forward( block );
					if ( block->token->type != OPEN_PARENTHESIS )
						goto onError;

					Forward( block );
					if ( block->token->type != NUMBER )
						goto onError;

					fgdFile->mapSize.x = atoi( block->token->string );

					Forward( block );
					if ( block->token->type != COMMA )
						goto onError;

					Forward( block );
					if ( block->token->type != NUMBER )
						goto onError;

					fgdFile->mapSize.y = atoi( block->token->string );

					Forward( block );
					if ( block->token->type != CLOSE_PARENTHESIS )
						goto onError;

					continue;
				}

				if ( strcasecmp( block->token->string, "@AutoVisgroup" ) == 0 )
				{
					Forward( block );
					if ( block->token->type != EQUALS )
						goto onError;

					Forward( block );
					if ( block->token->type != STRING )
						goto onError;

					AssignOrResizeArray( fgdFile->autoVisGroups, AutoVIsGroup_t *, fgdFile->visGroupCount );

					AutoVIsGroup_t *visGroup = fgdFile->autoVisGroups[fgdFile->visGroupCount - 1] = malloc( sizeof( AutoVIsGroup_t ) );
					memset( visGroup, 0, sizeof( AutoVIsGroup_t ) );

					visGroup->name = strdup( block->token->string );

					Forward( block );
					if ( block->token->type != OPEN_BRACKET )
						goto onError;

					Forward( block );
					if ( block->token->type != STRING && block->token->type != CLOSE_BRACKET )
						goto onError;

					while ( block->token->type == STRING )
					{
						AssignOrResizeArray( visGroup->children, AutoVisGroupChild_t *, visGroup->childCount );
						AutoVisGroupChild_t *visGroupChild = visGroup->children[visGroup->childCount - 1] = malloc( sizeof( AutoVisGroupChild_t ) );
						memset( visGroupChild, 0, sizeof( AutoVisGroupChild_t ) );

						visGroupChild->name = strdup( block->token->string );

						Forward( block );
						if ( block->token->type != OPEN_BRACKET )
							goto onError;

						Forward( block );
						if ( block->token->type != STRING && block->token->type != CLOSE_BRACKET )
							goto onError;

						while ( block->token->type == STRING )
						{
							if ( block->token->type != STRING )
								goto onError;

							AssignOrResizeArray( visGroupChild->children, char *, visGroupChild->childCount );
							visGroupChild->children[visGroupChild->childCount - 1] = strdup( block->token->string );

							Forward( block );
						}

						if ( block->token->type != CLOSE_BRACKET )
							goto onError;

						Forward( block );
					}

					if ( block->token->type != CLOSE_BRACKET )
						goto onError;

					continue;
				}

				if ( strcasecmp( block->token->string, "@include" ) == 0 )
				{
					Forward( block );

					if ( block->token->type != STRING )
						goto onError;

					AssignOrResizeArray( fgdFile->includes, char *, fgdFile->includeCount );
					fgdFile->includes[fgdFile->includeCount - 1] = strdup( block->token->string );
					continue;
				}

				if ( strcasecmp( block->token->string, "@MaterialExclusion" ) == 0 )
				{
					Forward( block );

					if ( block->token->type != OPEN_BRACKET )
						goto onError;

					Forward( block );

					while ( block->token->type == STRING )
					{
						AssignOrResizeArray( fgdFile->materialExclusions, char *, fgdFile->materialExcludeCount );
						fgdFile->materialExclusions[fgdFile->materialExcludeCount - 1] = strdup( block->token->string );

						Forward( block );
					}

					if ( block->token->type != CLOSE_BRACKET )
						goto onError;

					continue;
				}

				if ( EndsWith( block->token->string, "Class" ) )
				{
					AssignOrResizeArray( fgdFile->entities, Entity_t *, fgdFile->entityCount );
					Entity_t *entity = fgdFile->entities[fgdFile->entityCount - 1] = malloc( sizeof( Entity_t ) );
					memset( entity, 0x0, sizeof( Entity_t ) );
					entity->classPropertyCount = 0;
					entity->entityPropertyCount = 0;
					entity->IOCount = 0;

					entity->type = strdup( block->token->string );

					Forward( block );

					while ( block->token->type == LITERAL )
					{
						AssignOrResizeArray( entity->classProperties, ClassProperties_t *, entity->classPropertyCount );
						ClassProperties_t *classProperties = entity->classProperties[entity->classPropertyCount - 1] = malloc( sizeof( ClassProperties_t ) );

						classProperties->name = strdup( block->token->string );
						classProperties->classPropertyCount = 0;
						classProperties->classProperties = NULL;

						Forward( block );
						if ( block->token->type == OPEN_PARENTHESIS )
						{
							// if there are more than 40 non comma separated parameters, you're doing something wrong.
							// The value is already so high in case anyone adds new fgd class parameters in the future that require them.
							char *fields[40];
							memset( fields, 0x0, sizeof( char * ) * 40 );

							int i = 0;

							Forward( block );
							while ( block->token->type == LITERAL || block->token->type == COMMA || block->token->type == STRING || block->token->type == NUMBER )
							{
								if ( i > 40 ) // wtf happened?
									goto onError;

								if ( block->token->type == COMMA )
								{
									// AssignOrResizeArray( classProperties->classProperties, ClassProperty_t*, classProperties->classPropertyCount );
									if ( classProperties->classProperties != NULL )
									{
										classProperties->classPropertyCount++;
										classProperties->classProperties = realloc( classProperties->classProperties, ( sizeof( ClassProperty_t * ) * classProperties->classPropertyCount ) );
									}
									else
									{
										classProperties->classProperties = malloc( sizeof( ClassProperty_t * ) );
										classProperties->classPropertyCount++;
									}

									ClassProperty_t *property = classProperties->classProperties[classProperties->classPropertyCount - 1] = malloc( sizeof( ClassProperty_t ) );
									property->propertyCount = i;
									property->properties = malloc( sizeof( char * ) * i );
									for ( int j = 0; j < i; j++ )
									{
										property->properties[j] = strdup( fields[j] );
									}

									for ( int j = 0; j < i; j++ )
									{
										free( fields[j] );
									}
									i = 0;
									Forward( block );
									continue;
								}

								fields[i] = strdup( block->token->string );
								i++;

								Forward( block );
							}

							if ( i > 0 )
							{
								AssignOrResizeArray( classProperties->classProperties, ClassProperty_t *, classProperties->classPropertyCount );
								ClassProperty_t *property = classProperties->classProperties[classProperties->classPropertyCount - 1] = malloc( sizeof( ClassProperty_t ) );
								property->propertyCount = i;
								property->properties = malloc( sizeof( char * ) * i );
								for ( int j = 0; j < i; j++ )
								{
									property->properties[j] = strdup( fields[j] );
								}

								for ( int j = 0; j < i; j++ )
								{
									free( fields[j] );
								}
								i = 0;
								Forward( block );
								continue;
							}

							if ( block->token->type != CLOSE_PARENTHESIS )
								goto onError;

							Forward( block );
						}
					}

					if ( block->token->type != EQUALS )
						goto onError;

					Forward( block );
					if ( block->token->type != LITERAL )
						goto onError;

					entity->entityName = strdup( block->token->string );

					Forward( block );

					if ( block->token->type == COLUMN )
					{
						Forward( block );

						if ( block->token->type != STRING )
							goto onError;

						if ( !processFGDStrings( &block, &entity->entityDescription ) )
							goto onError;
					}

					if ( block->token->type != OPEN_BRACKET )
						goto onError;

					Forward( block );
					while ( block->token->type != CLOSE_BRACKET )
					{
						if ( block->token->type != LITERAL )
							goto onError;

						if ( strcasecmp( block->token->string, "input" ) == 0 || strcasecmp( block->token->string, "output" ) == 0 )
						{
							AssignOrResizeArray( entity->inputOutput, InputOutput_t *, entity->IOCount );
							InputOutput_t *inputOutput = entity->inputOutput[entity->IOCount - 1] = malloc( sizeof( InputOutput_t ) );
							inputOutput->description = NULL;
							inputOutput->name = NULL;

							inputOutput->putType = strcasecmp( block->token->string, "input" ) == 0 ? INPUT : OUTPUT;

							Forward( block );

							inputOutput->name = strdup( block->token->string );

							Forward( block );

							if ( block->token->type != OPEN_PARENTHESIS )
								goto onError;

							Forward( block );

							if ( block->token->type != LITERAL )
								goto onError;

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

							Forward( block );

							if ( block->token->type != CLOSE_PARENTHESIS )
								goto onError;

							Forward( block );

							if ( block->token->type == COLUMN )
							{
								Forward( block );

								if ( block->token->type != STRING )
									goto onError;

								if ( !processFGDStrings( &block, &inputOutput->description ) )
									goto onError;
							}

							continue;
						}
						else
						{
							AssignOrResizeArray( entity->entityProperties, EntityProperties_t *, entity->entityPropertyCount );
							EntityProperties_t *entityProperties = entity->entityProperties[entity->entityPropertyCount - 1] = malloc( sizeof( EntityProperties_t ) );
							memset( entityProperties, 0x0, sizeof( EntityProperties_t ) );
							entityProperties->flagCount = 0;
							entityProperties->choiceCount = 0;
							entityProperties->readOnly = false;
							entityProperties->reportable = false;

							entityProperties->propertyName[32] = '\0'; // last character should always be a null terminator.
							strncpy( entityProperties->propertyName, block->token->string, 31 );

							Forward( block );
							if ( block->token->type != OPEN_PARENTHESIS )
								goto onError;

							Forward( block );
							if ( block->token->type != LITERAL )
								goto onError;

							entityProperties->type = strdup( block->token->string );

							Forward( block );
							if ( block->token->type != CLOSE_PARENTHESIS )
								goto onError;

							Forward( block );

							if ( strcasecmp( block->token->string, "readonly" ) == 0 )
							{
								entityProperties->readOnly = true;
								Forward( block );
							}

							if ( ( strcasecmp( block->token->string, "*" ) == 0 || strcasecmp( block->token->string, "report" ) == 0 ) )
							{
								entityProperties->reportable = true;
								Forward( block );
							}

							// TODO: fix this shit dawg.

							if ( block->token->type == EQUALS )
							{
								goto isFOC;
							}

							if ( block->token->type != COLUMN )
								continue;

							Forward( block );

							if ( block->token->type != STRING )
								goto onError;

							entityProperties->displayName = strdup( block->token->string );

							Forward( block );

							if ( block->token->type == EQUALS )
							{
								goto isFOC;
							}

							if ( block->token->type != COLUMN )
								continue;

							Forward( block );

							if ( block->token->type != COLUMN )
							{
								entityProperties->defaultValue = strdup( block->token->string );
								Forward( block );
							}

							if ( block->token->type == EQUALS )
							{
								goto isFOC;
							}

							if ( block->token->type != COLUMN )
								continue;

							Forward( block );

							if ( block->token->type != STRING )
								goto onError;

							if ( !processFGDStrings( &block, &entityProperties->propertyDescription ) )
								goto onError;

							if ( block->token->type == EQUALS )
							{
								goto isFOC;
							}

							continue;

						isFOC:
						{
							bool isFlags = strcasecmp( entityProperties->type, "flags" ) == 0;

							Forward( block );

							if ( block->token->type != OPEN_BRACKET )
								goto onError;

							Forward( block );

							while ( block->token->type != CLOSE_BRACKET )
							{
								if ( isFlags && block->token->type != NUMBER )
									goto onError;

								if ( isFlags )
								{
									AssignOrResizeArray( entityProperties->flags, flags_t *, entityProperties->flagCount );
									flags_t *flags = entityProperties->flags[entityProperties->flagCount - 1] = malloc( sizeof( flags_t ) );
									flags->value = atoi( block->token->string );

									Forward( block );
									if ( block->token->type != COLUMN )
										goto onError;

									Forward( block );
									if ( block->token->type != STRING )
										goto onError;

									flags->displayName = strdup( block->token->string );

									if ( getNext(block)->token->type == COLUMN )
									{
										Forward( block );

										Forward( block );
										if ( block->token->type != NUMBER )
											goto onError;
										flags->checked = strcasecmp( block->token->string, "1" ) == 0;
									}

									Forward( block );
								}
								else
								{
									AssignOrResizeArray( entityProperties->choices, choice_t *, entityProperties->choiceCount );
									choice_t *choice = entityProperties->choices[entityProperties->choiceCount - 1] = malloc( sizeof( choice_t ) );
									choice->value = strdup( block->token->string );

									Forward( block );
									if ( block->token->type != COLUMN )
										goto onError;

									Forward( block );
									if ( block->token->type != STRING )
										goto onError;

									choice->displayName = strdup( block->token->string );

									Forward( block );
								}
							}
						}
						}

						Forward( block );
					}
				}
			}
		}
	}

	free( ownedChar );
	freeTokenizer( tokenizer );

	return fgdFile;

onError:
	freeFGDFile( fgdFile );
	free( ownedChar );
	freeTokenizer( tokenizer );
	*err = PARSE_ERROR;
	return fgdFile;
}

bool processFGDStrings( TokenBlock_t **block, char **str )
{
	char *fields[50][MAX_STR_CHUNK_LENGTH];

	int index = 0;
	while ( ( *block )->token->type == STRING )
	{
		int len = strlen( ( *block )->token->string );

		if ( len > MAX_STR_CHUNK_LENGTH )
			return false;

		memset(fields[index], 0, MAX_STR_CHUNK_LENGTH);

		strncpy( (char *)fields[index], ( *block )->token->string, MAX_STR_CHUNK_LENGTH );

		index++;
		Forward( ( *block ) );
		if ( ( *block )->token->type == PLUS )
		{
			Forward( ( *block ) );
		}
	}

	char *descString = *str = malloc( index * MAX_STR_CHUNK_LENGTH );
	memset( fields[index], 0, index * MAX_STR_CHUNK_LENGTH );

	for ( int d = 0; d < index; d++ )
	{
		if(d == 0)
			strncpy(descString, fields[d], MAX_STR_CHUNK_LENGTH);
		else
			strncat( descString, fields[d], MAX_STR_CHUNK_LENGTH );
	}

	return true;
}

void freeFGDFile( FGDFile_t *file )
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

		for ( int p = 0; p < file->entities[i]->IOCount; p++ )
		{
			if ( file->entities[i]->inputOutput[p]->name )
				free( file->entities[i]->inputOutput[p]->name );

			if ( file->entities[i]->inputOutput[p]->description )
				free( file->entities[i]->inputOutput[p]->description );

			if(file->entities[i]->inputOutput[p]->stringType)
				free(file->entities[i]->inputOutput[p]->stringType);

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

			if ( file->entities[i]->entityProperties[p]->choices )
			{
				for ( int j = 0; j < file->entities[i]->entityProperties[p]->choiceCount; j++ )
				{
					if ( file->entities[i]->entityProperties[p]->choices[j]->value )
						free( file->entities[i]->entityProperties[p]->choices[j]->value );
					if ( file->entities[i]->entityProperties[p]->choices[j]->displayName )
						free( file->entities[i]->entityProperties[p]->choices[j]->displayName );

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
TokenBlock_t *getNext( TokenBlock_t *block )
{
	block = block->next;

	if ( !block )
		return NULL;
	while ( block->token->type == COMMENT )
		block = block->next;

	return block;
}

char *getNextString( char *file, int *amount )
{
	for ( ; *file != '"'; ++file )
		;
	++file;
	int i = 0;
	for ( ; *file != '"'; ++file, ++i )
		;
	char *name = malloc( sizeof( char ) * ( i + 1 ) );
	strncpy( name, file - i, i );
	name[i] = '\0';
	*amount = i + 1;
	return name;
}
