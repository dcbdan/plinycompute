#ifndef _LEXER_H_
#define _LEXER_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

    #define YYSTYPE TTSTYPE

	struct TTLexerExtra {
		char errorMessage[500];
	};

	typedef void* yyscan_t;
	int ttlex_init_extra(struct TTLexerExtra *, yyscan_t *);
	int ttlex_destroy(yyscan_t);

	typedef struct yy_buffer_state *YY_BUFFER_STATE;
	YY_BUFFER_STATE tt_scan_string (const char *, yyscan_t);
	void tt_delete_buffer (YY_BUFFER_STATE, yyscan_t);

	struct NProgram;
	void tterror(yyscan_t, struct NProgram **myStatement, const char *);

	union TTSTYPE;
	int ttlex(union TTSTYPE *, yyscan_t);

//#ifdef __cplusplus
//}
//#endif

#endif
