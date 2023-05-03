#pragma once

enum ParseError
{
	NO_ERROR = 0,
	TOKENIZATION_ERROR,
	INVALID_DEFINITION,
	INVALID_EQUALS,
	INVALID_OPEN_BRACE,
	INVALID_CLOSE_BRACE,
	INVALID_OPEN_BRACKET,
	INVALID_CLOSE_BRACKET,
	INVALID_OPEN_PARENTHESIS,
	INVALID_CLOSE_PARENTHESIS,
	INVALID_COMMA,
	INVALID_STRING,
	INVALID_PLUS,
	INVALID_LITERAL,
	INVALID_COLUMN,
	INVALID_NUMBER,
	ALLOCATION_FAILURE,
	PREMATURE_EOF,
};