#include "tokenizer.h"

#include <ctype.h>
#include <malloc.h>
#include <string.h>

//private
char singleTokens[] = "{}[](),:=+";
TokenType_e valueTokens[] = { OPEN_BRACE, CLOSE_BRACE, OPEN_BRACKET, CLOSE_BRACKET, OPEN_PARENTHESIS, CLOSE_PARENTHESIS, COMMA, COLUMN, EQUALS, PLUS };
enum ParseError tokenErrors[] = { INVALID_OPEN_BRACE, INVALID_CLOSE_BRACE, INVALID_OPEN_BRACKET, INVALID_CLOSE_BRACKET, INVALID_OPEN_PARENTHESIS, INVALID_CLOSE_PARENTHESIS, INVALID_COMMA,INVALID_COLUMN,INVALID_EQUALS,INVALID_PLUS};

//Appends a new token to the tokenizer's token list.
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

// The method for getting a new Tokenizer Token.
Token_t *GenerateEmptyToken()
{
	Token_t *token = malloc( sizeof( Token_t ) );
	memset( token, 0, sizeof( Token_t ) );

	return token;
}

//public

// The method for getting a new Tokenizer.
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

/*
 * char* SanitizeString(const char* string, int length, int* newLength)
 * Removes any non printable characters from a string with the exception of
 * tabs and new lines. (\t, \n)
 */

char* SanitizeString(const char* string, int length, int* newLength)
{
	char* tmp = malloc( length + 1);
	int r = 0;
	for(int l = 0; l < length; l++)
	{
		if(!isprint(string[l]) && string[l] != '\n' && string[l] != '\t')
			continue;
		tmp[r] = string[l];
		r++;
	}

	tmp[r] = '\0';
	*newLength = r;
	char* ret = strndup(tmp, r);
	free(tmp);
	return ret;
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

	for ( int i = 0, ln = 1, pos = 1; eof != seditedFile; i++, seditedFile++, pos++ )
	{
		char c = *seditedFile;

		if ( c == '\t' )
			continue;

		if ( c == '\r' )
			continue;

		if ( c == '\n' )
		{
			ln++;
			pos = 1;
			continue;
		}

		if ( c == '"' )
		{
			int currentLine = ln;
			int currentLength = i;
			int currentPos = pos;

			char *currentPosition = seditedFile;
			c = '\t'; // We can get away with this to trick the while loop :)
			while ( c != '"' )
			{
				seditedFile++;
				pos++;
				c = *seditedFile;
				i++;
				if ( c == '\n' )
					ln++;
			}
			seditedFile++;
			i++;
			pos++;
			Token_t *token = GenerateEmptyToken();
			token->line = currentLine;
			token->type = STRING;
			token->associatedError = INVALID_STRING;

			int newStrLength = 0;
			token->string = SanitizeString( currentPosition, i - currentLength, &newStrLength );

			int subtractFromRange = (i - currentLength - newStrLength);

			Range_t range = { currentPos, pos - (currentPos - subtractFromRange ) };
			token->range = range;


			PushToTokenList( tokenizer, token );
			seditedFile--;
			i--;
			pos--;
			continue;
		}

		if ( c == '/' && *( seditedFile + 1 ) == '/' )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			int currentPos = pos;

			while ( c != '\n' )
			{
				c = *seditedFile;
				pos++;
				i++;
				seditedFile++;
			}
			seditedFile--;
			i--;
			pos--;

			Token_t *token = GenerateEmptyToken();
			token->line = ln;
			token->type = COMMENT;

			int newStrLength = 0;
			token->string = SanitizeString( currentPosition, i - currentLength, &newStrLength );

			int subtractFromRange = (i - currentLength - newStrLength);

			Range_t range = { currentPos, pos - (currentPos - subtractFromRange ) };
			token->range = range;

			PushToTokenList( tokenizer, token );

			seditedFile--;
			i--;
			pos--;
			continue;
		}

		if ( c == '@' )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			int currentPos = pos;

			while ( c != '\n' && c != '\t' && c != '\r' && c != ' ' && c != '(' )
			{
				c = *seditedFile;
				pos++;
				i++;
				seditedFile++;
			}
			seditedFile--;
			i--;
			pos--;

			if ( c == '\n' )
				ln++;
			Token_t *token = GenerateEmptyToken();
			token->line = ln;
			token->type = DEFINITION;
			token->associatedError = INVALID_DEFINITION;

			int newStrLength = 0;
			token->string = SanitizeString( currentPosition, i - currentLength, &newStrLength );

			int subtractFromRange = (i - currentLength - newStrLength);

			Range_t range = { currentPos, pos - (currentPos - subtractFromRange ) };
			token->range = range;

			PushToTokenList( tokenizer, token );

			seditedFile--;
			i--;
			pos--;
			continue;
		}

		if ( isdigit( c ) != 0 || ( c == '-' && isdigit( *( seditedFile + 1 ) ) ) )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			int currentPos = pos;

			if ( c == '-' )
			{
				seditedFile++;
				pos++;
				i++;
				c = *seditedFile;
			}

#ifdef SLOME_UNIFIED_FGD
			while ( isdigit( c ) != 0 || c == '.' )
#else
			while ( isdigit( c ) != 0 )
#endif
			{
				c = *seditedFile;
				i++;
				pos++;
				seditedFile++;
			}

			seditedFile--;
			i--;
			pos--;

			Token_t *token = GenerateEmptyToken();
			token->line = ln;
			token->type = NUMBER;
			token->associatedError = INVALID_NUMBER;
			Range_t range = { currentPos, pos };
			token->range = range;
			token->string = strndup( currentPosition, i - currentLength );
			PushToTokenList( tokenizer, token );
			seditedFile--;
			i--;
			pos--;
			continue;
		}

		char *valueKey = strchr( singleTokens, c );

		if ( valueKey )
		{
			int spaces = (int)( (int)( (char *)valueKey - (char *)singleTokens ) / sizeof( char ) ); // char should be 1, but I am sanity checking it anyway.
			TokenType_e tType = valueTokens[spaces];
			enum ParseError tParseError = tokenErrors[spaces];
			Token_t *token = GenerateEmptyToken();
			token->line = ln;
			token->type = tType;
			token->associatedError = tParseError;
			Range_t range = { pos, pos + 1 };
			token->range = range;
			char temp[] = { c };
			token->string = strndup( temp, 1 );
			PushToTokenList( tokenizer, token );
			continue;
		}

		if ( c != ' ' )
		{
			int currentLength = i;
			char *currentPosition = seditedFile;
			int currentPos = pos;

			while ( c != '\n' && c != ' ' && c != '\t' && c != '\r' && !strchr( singleTokens, c ) )
			{
				seditedFile++;
				pos++;
				c = *seditedFile;
				i++;
			}

			Token_t *token = GenerateEmptyToken();
			token->line = ln;
			token->type = LITERAL;
			token->associatedError = INVALID_LITERAL;

			int newStrLength = 0;
			token->string = SanitizeString( currentPosition, i - currentLength, &newStrLength );

			int subtractFromRange = (i - currentLength - newStrLength);

			Range_t range = { currentPos, pos - (currentPos - subtractFromRange ) };
			token->range = range;

			PushToTokenList( tokenizer, token );
			seditedFile--;
			i--;
			pos--;
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