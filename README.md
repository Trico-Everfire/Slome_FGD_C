# Slome_FGD_C
The Slome FGD Parser is a FGD (Forge Game Data) parser written in C.

Developed and maintained by Trico Everfire.

Any additional PRs are welcome.

Example:
```c

// Basic get the text file contents.
FILE *file = fopen( "path/to/file.fgd", "r" );

fseek( file, 0, SEEK_END );
size_t size = ftell( file );
char *fileContents = (char *)malloc( size );

rewind( file );
fread( fileContents, sizeof( char ), size, file );

fclose( file );

ParsingError_t err;

// We turn the raw text file into a FGD struct.
// if this process fails it returns NULL.
// The memory in that struct needs to be cleared
// after we're done with the file.
// FreeFGDFile is used for this.
    
struct FGDFile *fgdFile = ParseFGDFile( fileContents, size, &err );

if ( err.err != NO_ERROR )
{
    printf( "ERROR Enum Type: %i\n", err.err );
    printf( "At line: %i\n", err.line );
    printf( "From line index: %i to %i\n", err.span.x, err.span.y );

    free( fileContents );
    return 1;
}

//... your code here.

free( fileContents );
FreeFGDFile( fgdFile );
```

### ParseFGDFile
Provides a struct with all entities, the map size, material exclusions, visgroups and includes.
```c 
struct FGDFile *ParseFGDFile( char *file, size_t fileLength, ParsingError_t *err );
```

### FreeFGDFile
clears up memory inside FGDFile.
```c
void FreeFGDFile( struct FGDFile *file );
```


