/*
 * scan.c
 * Lexical scanner.
 * $Id: scan.c 139 2008-07-16 09:56:31Z catseye $
 */

#include <stdarg.h>

#include "lib.h"

#include "stream.h"
#include "file.h"

#include "scan.h"
#include "report.h"
#include "render.h"

enum token_type {
	TOKEN_EOF,
	TOKEN_NUMBER,
	TOKEN_BAREWORD,
	TOKEN_SYMBOL,
	TOKEN_QUOTED_STRING
};

struct scanner {
        struct reporter *reporter;
	struct process	*input;		/* file process from which we are scanning */
	const char	*filename;	/* name of file scanning from */
	char		*token;		/* text content of token we just scanned */
	enum token_type	 token_type;	/* type of token that was scanned */
	unsigned int	 token_length;	/* length of the token that was scanned */
	int	 	 line;		/* current line number, 1-based */
	int	 	 column;	/* current column number, 1-based */
	int	 	 last_column;	/* for putback */
	char		*putback_buf;	/* buffer of characters put back */
	int		 putback_pos;	/* position within putback buffer */
};

#define	PUTBACK_SIZE	80

struct scanner *
scanner_new(struct reporter *r)
{
	struct scanner *sc;

	if ((sc = malloc(sizeof(struct scanner))) == NULL) {
		return NULL;
	}
	if ((sc->token = malloc(256 * sizeof(char))) == NULL) {
		free(sc);
		return NULL;
	}

        sc->reporter = r;
	sc->filename = NULL;
	sc->input = NULL;
	sc->putback_buf = malloc(PUTBACK_SIZE * sizeof(char));
	sc->putback_pos = 0;

	return sc;
}

void
scanner_free(struct scanner *sc)
{
	scanner_close(sc);
	free(sc->token);
	free(sc);
}

void
scanner_reset(struct scanner *sc)
{
	sc->line = 1;
	sc->column = 1;
	sc->last_column = 0;
	scanner_scan(sc);		/* prime the pump */
}

/*
 * caller is responsible for freeing the filename
 */
int
scanner_open(struct scanner *sc, const char *filename)
{
	sc->filename = filename;
	if ((sc->input = file_open(filename, "r")) == NULL) {
		scanner_report(sc, REPORT_ERROR,
		    "Can't open '%s' for reading", filename);
		return 0;
	}
	scanner_reset(sc);
	return 1;
}

/*
 * caller is responsible for freeing the filename
 */
int
scanner_attach(struct scanner *sc, struct process *p, const char *filename)
{
	sc->filename = filename;
	sc->input = p;
	scanner_reset(sc);
	return 1;
}

void
scanner_close(struct scanner *sc)
{
	if (sc->filename != NULL) {
		sc->filename = NULL;
	}
	if (sc->input != NULL) {
		stream_close(NULL, sc->input);
		sc->input = NULL; /* ? */
	}
}

/*
 * x is not a string, it is a pointer to a single character.
 */
static void
scan_char(struct scanner *sc, char *x)
{
	sc->last_column = sc->column;

	/* do a 'getc' */
	if (sc->putback_pos > 0) {
		*x = sc->putback_buf[sc->putback_pos--];
	} else {
		stream_read(NULL, sc->input, x, sizeof(char));
	}

	if (*x == '\n') {
		sc->column = 1;
		sc->line++;
	} else if (*x == '\t') {
		sc->column++;
		while (sc->column % 8 != 0)
			sc->column++;
	} else {
		sc->column++;
	}
}

static void
putback(struct scanner *sc, char x)
{
	if (stream_is_at_end(NULL, sc->input))
		return;

	/* do a 'ungetc' */
	if (sc->putback_pos < (PUTBACK_SIZE - 1)) {
		sc->putback_buf[++sc->putback_pos] = x;
	} else {
		scanner_report(sc, REPORT_ERROR,
		    "Putback buffer size exceeded on '%s'", sc->filename);
	}

	sc->column = sc->last_column;
	if (x == '\n')
		sc->line--;
}

static void
real_scan(struct scanner *sc)
{
	char x;
	int i = 0;

	sc->token[0] = '\0';
	sc->token_length = 0;
	if (stream_is_at_end(NULL, sc->input)) {
		sc->token_type = TOKEN_EOF;
		return;
	}

	scan_char(sc, &x);

	/* Skip whitespace. */

top:
	while (k_isspace(x) && !stream_is_at_end(NULL, sc->input)) {
		scan_char(sc, &x);
	}

	/* Skip comments. */

	if (x == '/') {
		scan_char(sc, &x);
		if (x == '/') {
			while (x != '\n' && !stream_is_at_end(NULL, sc->input)) {
				scan_char(sc, &x);
			}
			goto top;
		} else {
			putback(sc, x);
			x = '/';
			/* falls through to the bottom of scan() */
		}
	}

	if (stream_is_at_end(NULL, sc->input)) {
		sc->token[0] = '\0';
		sc->token_type = TOKEN_EOF;
		return;
	}

	/*
	 * Scan decimal numbers.  Must start with a
	 * digit (not a sign or decimal point.)
	 */
	if (k_isdigit(x)) {
		while ((k_isdigit(x) || x == '.') && !stream_is_at_end(NULL, sc->input)) {
			sc->token[i++] = x;
			sc->token_length++;
			scan_char(sc, &x);
		}
		putback(sc, x);
		sc->token[i] = '\0';
		sc->token_type = TOKEN_NUMBER;
		return;
	}

	/*
	 * Scan quoted strings.
	 */
	if (x == '"') {
		scan_char(sc, &x);
		while (x != '"' && !stream_is_at_end(NULL, sc->input) && i < 255) {
			sc->token[i++] = x;
			sc->token_length++;
			scan_char(sc, &x);
		}
		sc->token[i] = '\0';
		sc->token_type = TOKEN_QUOTED_STRING;
		return;
	}

	/*
	 * Scan alphanumeric ("bareword") tokens.
	 */
	if (k_isalpha(x) || x == '_') {
		while ((k_isalpha(x) || k_isdigit(x) || x == '_') && !stream_is_at_end(NULL, sc->input)) {
			sc->token[i++] = x;
			sc->token_length++;
			scan_char(sc, &x);
		}
		putback(sc, x);
		sc->token[i] = '\0';
		sc->token_type = TOKEN_BAREWORD;
		return;
	}

	/*
	 * Scan multi-character symbols.
	 */
	if (x == '>' || x == '<' || x == '!') {
		sc->token[i++] = x;
		sc->token_length++;
		scan_char(sc, &x);
		if (x == '=' && !stream_is_at_end(NULL, sc->input)) {
			sc->token[i++] = x;
			sc->token_length++;
			scan_char(sc, &x);
		} else {
			putback(sc, x);
		}
		sc->token[i] = '\0';
		sc->token_type = TOKEN_SYMBOL;
		return;
	}

	/*
	 * Degenerate case: scan single symbols.
	 */
	sc->token[0] = x;
	sc->token[1] = '\0';
	sc->token_length = 1;
	sc->token_type = TOKEN_SYMBOL;
}

void
scanner_scan(struct scanner *sc)
{
	real_scan(sc);
#ifdef DEBUG
	printf("scanned -> '%s'\n", sc->token);
#endif
}

void
scanner_expect(struct scanner *sc, const char *x)
{
	if (strcmp(sc->token, x) == 0) {
		scanner_scan(sc);
	} else {
		scanner_report(sc, REPORT_ERROR, "Expected '%s'", x);
	}
}

void
scanner_scanline(struct scanner *sc)
{
	char x;

	scan_char(sc, &x);
	while (x != '\n' && !stream_is_at_end(NULL, sc->input)) {
		scan_char(sc, &x);
	}
	real_scan(sc);
}

int
scanner_tokeq(struct scanner *sc, const char *token)
{
        return strcmp(sc->token, token) == 0;
}

const char *
scanner_token_string(struct scanner *sc)
{
        return sc->token;
}

int
scanner_token_length(struct scanner *sc)
{
        return sc->token_length;
}

int
scanner_eof(struct scanner *sc)
{
        return sc->token_type == TOKEN_EOF;
}

const char *
scanner_filename(struct scanner *sc)
{
        return sc->filename != NULL ? sc->filename : "<no file>";
}

int
scanner_line(struct scanner *sc)
{
        return sc->line;
}

int
scanner_column(struct scanner *sc)
{
        return sc->column;
}

void
scanner_report(struct scanner *sc, enum report_type rtype, const char *fmt, ...)
{
	va_list args;

	/*
	 * Breaking abstraction just to have a nicely-formatted error message...
	 */
        process_render(reporter_stream(sc->reporter),
            "(%s, line %d, column %d, token '%s'): ",
            scanner_filename(sc), scanner_line(sc),
            scanner_column(sc), scanner_token_string(sc));

	va_start(args, fmt);
        report_va_list(sc->reporter, rtype, fmt, args);
	va_end(args);
}
