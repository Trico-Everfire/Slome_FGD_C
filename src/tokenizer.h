#pragma once

#include <stdbool.h>

#define firstOrNext( toNext, toNext2 ) toNext ? toNext->next : toNext2->first

#define Forward( block )                    \
	block = block->next;                    \
	if ( !block )                           \
		break;                              \
	while ( block->token->type == COMMENT ) \
		block = block->next;

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

Token_t *generateEmptyToken();

Tokenizer_t *GetNewTokenList();

void PushToTokenList( Tokenizer_t *tokeniser, Token_t *token );

void freeTokenizer( Tokenizer_t *tokeniser );
