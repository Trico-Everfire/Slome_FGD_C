#include "library.h"

#include <bits/types/FILE.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

int main( int argc, char **argv )
{

	//Syntax: -i "fpath-to-file"
	if( strcasecmp(argv[1], "-i") != 0 || !argv[2])
		return 1;

	FILE *f = fopen( argv[2], "r" );

	fseek( f, 0, SEEK_END );
	size_t size = ftell( f );
	char *fileContents = (char *)malloc( size );

	rewind( f );
	fread( fileContents, sizeof( char ), size, f );

	fclose( f );

	enum ParseError err;

	struct FGDFile *file = ParseFGDFile( fileContents, size, &err );

	if ( err == PARSE_ERROR )
	{
		free(fileContents);
		return 1;
	}

	printf( "Map width is: %i\n", file->mapSize.x );
	printf( "Map height is: %i\n", file->mapSize.y );

	printf( "Visgroup count is: %i\n", file->visGroupCount );

	for ( int i = 0; i < file->visGroupCount; i++ )
	{
		struct AutoVIsGroup *vis = file->autoVisGroups[i];
		printf( "Visgroup Name: %s \n", vis->name );
		for ( int j = 0; j < vis->childCount; j++ )
		{
			struct AutoVisGroupChild *visChild = vis->children[j];
			printf( "Visgroup Child Name: %s \n", visChild->name );
			for ( int k = 0; k < visChild->childCount; k++ )
				printf( "Visgroup Child Content: %s \n", visChild->children[k] );
		}
	}

	for ( int k = 0; k < file->materialExcludeCount; k++ )
		printf( "Material Exclusion Content: %s \n", file->materialExclusions[k] );



	for ( int i = 0; i < file->entityCount; i++ )
	{
		char *type = file->entities[i]->type;
		printf( "Entity Type is: %s\n", type );

		char *name = file->entities[i]->entityName;
		printf( "Entity name is: %s\n", name );

		if ( file->entities[i]->entityDescription )
			printf( "Entity Description is: %s\n", file->entities[i]->entityDescription );


		for( int j = 0; j < file->entities[i]->IOCount; j++)
		{

			if(file->entities[i]->inputOutput[j]->putType == INPUT)
			{
				printf( "Entity Input Name is: %s\n", file->entities[i]->inputOutput[j]->name );
				printf( "Entity Input Type is: %s\n", file->entities[i]->inputOutput[j]->stringType );
				if(file->entities[i]->inputOutput[j]->description)
					printf( "Entity Input Description is: %s\n", file->entities[i]->inputOutput[j]->description );
			}
			else
			{
				printf( "Entity Output Name is: %s\n", file->entities[i]->inputOutput[j]->name );
				printf( "Entity Output Type is: %s\n", file->entities[i]->inputOutput[j]->stringType );
				if(file->entities[i]->inputOutput[j]->description)
					printf( "Entity Output Description is: %s\n", file->entities[i]->inputOutput[j]->description );
			}
		}

		for ( int j = 0; j < file->entities[i]->classPropertyCount; j++ )
		{
			char *classPropertyName = file->entities[i]->classProperties[j]->name;
			printf( "Entity Property Name: %s\n", classPropertyName );
			for ( int k = 0; k < file->entities[i]->classProperties[j]->classPropertyCount; k++ )
			{
				ClassProperty_t *classPropertyArgument = file->entities[i]->classProperties[j]->classProperties[k];
				for ( int l = 0; l < classPropertyArgument->propertyCount; l++ )
				{
					printf( "Entity Property Argument: %s\n", classPropertyArgument->properties[l] );
				}
			}
		}

		for( int j = 0; j < file->entities[i]->entityPropertyCount; j++)
		{
			printf( "Entity Property Name: %s\n", file->entities[i]->entityProperties[j]->propertyName);
			if(file->entities[i]->entityProperties[j]->displayName)
				printf( "Entity Property Display Name: %s\n", file->entities[i]->entityProperties[j]->displayName);
			if(file->entities[i]->entityProperties[j]->propertyDescription)
				printf( "Entity Property Description: %s\n", file->entities[i]->entityProperties[j]->propertyDescription);
			if(file->entities[i]->entityProperties[j]->defaultValue)
				printf( "Entity Property Default Value: %s\n", file->entities[i]->entityProperties[j]->defaultValue);

			for( int k = 0; k < file->entities[i]->entityProperties[j]->choiceCount; k++)
			{
				printf("Entity Property Choices Value: %s\n", file->entities[i]->entityProperties[j]->choices[k]->value);
				printf("Entity Property Choices Name: %s\n", file->entities[i]->entityProperties[j]->choices[k]->displayName);
			}
			for( int k = 0; k < file->entities[i]->entityProperties[j]->flagCount; k++)
			{
				printf("Entity Property Flags Value: %d\n", file->entities[i]->entityProperties[j]->flags[k]->value);
				printf("Entity Property Flags Name: %s\n", file->entities[i]->entityProperties[j]->flags[k]->displayName);
				printf("Entity Property Flags Checked By Default: %d\n", file->entities[i]->entityProperties[j]->flags[k]->checked);

			}
		}

	}

	free( fileContents );
	FreeFGDFile( file );

	return 0;
}