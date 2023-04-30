#pragma once

#include <stdbool.h>

typedef enum TokenType
{

	COMMENT = 0,	   // //
	DEFINITION,		   // @something
	EQUALS,			   // =
	OPEN_BRACE,		   // {
	CLOSE_BRACE,	   // }
	OPEN_BRACKET,	   // [
	CLOSE_BRACKET,	   // ]
	OPEN_PARENTHESIS,  // (
	CLOSE_PARENTHESIS, // )
	COMMA,			   // ,
	STRING,			   // "something"
	PLUS,			   // +
	LITERAL,		   // anyything that isn't any of the other tokens.
	COLUMN,			   // :
	NUMBER,			   // numbers -200000 ... 0 ... 2000000

} TokenType_e;

typedef struct Range
{
	int start;
	int end;

} Range_t;

typedef struct Token
{
	TokenType_e type;
	Range_t range;
	char *string;
	int line;

} Token_t;

typedef struct TokenBlock
{
	Token_t *token;
	struct TokenBlock *next;

} TokenBlock_t;

typedef struct Tokenizer
{
	int tokenListCount;
	TokenBlock_t *first;
	TokenBlock_t *next;
} Tokenizer_t;

Tokenizer_t *GetNewTokenList();

bool TokenizeFile( char *file, unsigned long fileLength, Tokenizer_t **pTokenizer );

void FreeTokenizer( Tokenizer_t *tokeniser );