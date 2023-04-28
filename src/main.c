#include "library.h"

#include <bits/types/FILE.h>
#include <malloc.h>
#include <stdio.h>

int main( int argc, char **argv )
{
	// /home/trico/.local/share/Steam/steamapps/common/Portal 2/bin/portal2.fgd
	// /home/trico/.local/share/Steam/steamapps/common/Portal 2 Community Edition/p2ce/p2ce.fgd
	FILE *f = fopen( "/home/trico/.local/share/Steam/steamapps/common/Portal 2/bin/base.fgd", "r" );

	fseek( f, 0, SEEK_END );
	size_t size = ftell( f );
	char *fileContents = (char *)malloc( size );

	rewind( f );
	fread( fileContents, sizeof( char ), size, f );

	fclose( f );

	enum ParseError err;

	struct FGDFile *file = parseFGDFile( fileContents, size, &err );

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
	}

	free( fileContents );
	freeFGDFile( file );

	return 0;
}