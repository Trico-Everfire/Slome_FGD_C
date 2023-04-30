#include "tokenizer.h"

#include <malloc.h>
#include <string.h>

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