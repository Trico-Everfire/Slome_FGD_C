#include "include/libslome.h"

#include <bits/types/FILE.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv )
{
	// Syntax: -i "fpath-to-file"
	if ( strcasecmp( argv[1], "-i" ) != 0 || !argv[2] )
		return 1;

	FILE *file = fopen( argv[2], "r" );

	fseek( file, 0, SEEK_END );
	size_t size = ftell( file );
	char *fileContents = (char *)malloc( size );

	rewind( file );
	fread( fileContents, sizeof( char ), size, file );

	fclose( file );

	enum ParseError err;

	struct FGDFile *fgdFile = ParseFGDFile( fileContents, size, &err );

	if ( err != NO_ERROR )
	{
		free( fileContents );
		return 1;
	}

	printf( "Map width is: %i\n", fgdFile->mapSize.x );
	printf( "Map height is: %i\n", fgdFile->mapSize.y );

	printf( "Visgroup count is: %i\n", fgdFile->visGroupCount );

	for ( int i = 0; i < fgdFile->visGroupCount; i++ )
	{
		struct AutoVIsGroup *vis = fgdFile->autoVisGroups[i];
		printf( "Visgroup Name: %s \n", vis->name );
		for ( int j = 0; j < vis->childCount; j++ )
		{
			struct AutoVisGroupChild *visChild = vis->children[j];
			printf( "Visgroup Child Name: %s \n", visChild->name );
			for ( int k = 0; k < visChild->childCount; k++ )
				printf( "Visgroup Child Content: %s \n", visChild->children[k] );
		}
	}

	for ( int k = 0; k < fgdFile->materialExcludeCount; k++ )
		printf( "Material Exclusion Content: %s \n", fgdFile->materialExclusions[k] );

	for ( int i = 0; i < fgdFile->entityCount; i++ )
	{
		char *type = fgdFile->entities[i]->type;
		printf( "Entity Type is: %s\n", type );

		char *name = fgdFile->entities[i]->entityName;
		printf( "Entity name is: %s\n", name );

		if ( fgdFile->entities[i]->entityDescription )
			printf( "Entity Description is: %s\n", fgdFile->entities[i]->entityDescription );

		for ( int j = 0; j < fgdFile->entities[i]->IOCount; j++ )
		{
			if ( fgdFile->entities[i]->inputOutput[j]->putType == INPUT )
			{
				printf( "Entity Input Name is: %s\n", fgdFile->entities[i]->inputOutput[j]->name );
				printf( "Entity Input Type is: %s\n", fgdFile->entities[i]->inputOutput[j]->stringType );
				if ( fgdFile->entities[i]->inputOutput[j]->description )
					printf( "Entity Input Description is: %s\n", fgdFile->entities[i]->inputOutput[j]->description );
			}
			else
			{
				printf( "Entity Output Name is: %s\n", fgdFile->entities[i]->inputOutput[j]->name );
				printf( "Entity Output Type is: %s\n", fgdFile->entities[i]->inputOutput[j]->stringType );
				if ( fgdFile->entities[i]->inputOutput[j]->description )
					printf( "Entity Output Description is: %s\n", fgdFile->entities[i]->inputOutput[j]->description );
			}
		}

		for ( int j = 0; j < fgdFile->entities[i]->classPropertyCount; j++ )
		{
			char *classPropertyName = fgdFile->entities[i]->classProperties[j]->name;
			printf( "Entity Property Name: %s\n", classPropertyName );
			for ( int k = 0; k < fgdFile->entities[i]->classProperties[j]->classPropertyCount; k++ )
			{
				ClassProperty_t *classPropertyArgument = fgdFile->entities[i]->classProperties[j]->classProperties[k];
				for ( int l = 0; l < classPropertyArgument->propertyCount; l++ )
				{
					printf( "Entity Property Argument: %s\n", classPropertyArgument->properties[l] );
				}
			}
		}

		for ( int j = 0; j < fgdFile->entities[i]->entityPropertyCount; j++ )
		{
			printf( "Entity Property Name: %s\n", fgdFile->entities[i]->entityProperties[j]->propertyName );
			if ( fgdFile->entities[i]->entityProperties[j]->displayName )
				printf( "Entity Property Display Name: %s\n", fgdFile->entities[i]->entityProperties[j]->displayName );
			if ( fgdFile->entities[i]->entityProperties[j]->propertyDescription )
				printf( "Entity Property Description: %s\n", fgdFile->entities[i]->entityProperties[j]->propertyDescription );
			if ( fgdFile->entities[i]->entityProperties[j]->defaultValue )
				printf( "Entity Property Default Value: %s\n", fgdFile->entities[i]->entityProperties[j]->defaultValue );

			for ( int k = 0; k < fgdFile->entities[i]->entityProperties[j]->choiceCount; k++ )
			{
				printf( "Entity Property Choices Value: %s\n", fgdFile->entities[i]->entityProperties[j]->choices[k]->value );
				printf( "Entity Property Choices Name: %s\n", fgdFile->entities[i]->entityProperties[j]->choices[k]->displayName );
			}
			for ( int k = 0; k < fgdFile->entities[i]->entityProperties[j]->flagCount; k++ )
			{
				printf( "Entity Property Flags Value: %d\n", fgdFile->entities[i]->entityProperties[j]->flags[k]->value );
				printf( "Entity Property Flags Name: %s\n", fgdFile->entities[i]->entityProperties[j]->flags[k]->displayName );
				printf( "Entity Property Flags Checked By Default: %d\n", fgdFile->entities[i]->entityProperties[j]->flags[k]->checked );
			}
		}
	}

	free( fileContents );
	FreeFGDFile( fgdFile );

	return 0;
}