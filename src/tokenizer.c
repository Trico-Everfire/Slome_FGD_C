#include "tokenizer.h"

#include <ctype.h>
#include <malloc.h>
#include <string.h>

//private
char singleTokens[] = "{}[](),:=+";
TokenType_e valueTokens[] = { OPEN_BRACE, CLOSE_BRACE, OPEN_BRACKET, CLOSE_BRACKET, OPEN_PARENTHESIS, CLOSE_PARENTHESIS, COMMA, COLUMN, EQUALS, PLUS };

void PushToTokenList( Tokenizer_t *tokeniser, Token_t *token )
{
	TokenBlock_t *tokenList = malloc( sizeof( TokenBlock_t ) );

	tokenList->token = token;
	tokenList->next = NULL;

	if ( !tokeniser->first )
	{
		tokeniser->next = tokeniser->first = tokenList;
		tokeniser->tokenListCount++;
		return;
	}

	tokeniser->next = tokeniser->next->next = tokenList;
	tokeniser->tokenListCount++;
}

Token_t *GenerateEmptyToken()
{
	Token_t *token = malloc( sizeof( Token_t ) );
	memset( token, 0, sizeof( Token_t ) );

	return token;
}

//public
Tokenizer_t *GetNewTokenList()
{
	Tokenizer_t *pTokenizer = malloc( sizeof( Tokenizer_t ) );

	if ( !pTokenizer )
		return NULL;

	pTokenizer->next = NULL;
	pTokenizer->first = NULL;
	pTokenizer->tokenListCount = 0;

	return pTokenizer;
}

bool TokenizeFile( char *file, size_t fileLength, Tokenizer_t **pTokenizer )
{

	if(fileLength == 0)
		return false;

	if(!file)
		return false;

	if(!pTokenizer || !*(pTokenizer))
		return false;

	Tokenizer_t *tokenizer = *(pTokenizer);

	char *seditedFile = file;
	char *eof = seditedFile + fileLength;

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
			int currentLine = ln;
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
			Token_t *commentToken = GenerateEmptyToken();
			commentToken->line = currentLine;
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

			Token_t *commentToken = GenerateEmptyToken();
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
			Token_t *commentToken = GenerateEmptyToken();
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

			Token_t *commentToken = GenerateEmptyToken();
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
			Token_t *commentToken = GenerateEmptyToken();
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

			Token_t *commentToken = GenerateEmptyToken();
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

	return true;
}

void FreeTokenizer( Tokenizer_t *tokeniser )
{
	TokenBlock_t *block = tokeniser->first;

	while ( block )
	{
		free( block->token->string );
		free( block->token );
		TokenBlock_t *temp = block->next;
		free( block );
		block = temp;
	}

	free( tokeniser );
}