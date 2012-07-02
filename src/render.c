/*
 * render.c
 * printf-like formatting to a stream-like process.
 */

#include <stdarg.h>

#include "lib.h"
#include "stream.h"

#include "render.h"

static unsigned int
extract_int(const char *str, unsigned int *pos)
{
	unsigned int val = 0;

	while (k_isdigit(str[*pos])) {
		val = val * 10 + (str[*pos] - '0');
		(*pos)++;
	}

	return val;
}

/*
 * Note that there is no trailing NUL in the buf!
 */
static unsigned int
render_int(char *buf, unsigned int bufsize, int val)
{
	int pos = bufsize - 1;
	int sign = 0;

	if (val == 0) {
		buf[pos] = '0';
		return pos;
	}
	if (val < 0) {
		val *= -1;
		sign = -1;
	}
	while (pos >= 0 && val > 0) {
		buf[pos] = (char)((val % 10) + '0');
		pos--;
		val = val / 10;
	}
	if (pos >= 0 && sign == -1) {
		buf[pos] = '-';
		pos--;
	}
	if (pos < 0) {
		buf[0] = '?';	/* signify incorrectly rendered number */
	}

	return pos + 1;
}

#define MAX_DIGITS	32

/*
 * Similar to, but different from, C's printf().
 * The size specifier of each field gives the maximum number
 * of characters that will be rendered into that field, but
 * does not (at the moment) cause the field to have a fixed
 * display width.
 */
void
process_render(struct process *p, const char *fmt, ...)
{
	unsigned int pos = 0, length;
	va_list args;

	assert(p != NULL);
	va_start(args, fmt);

	while (fmt[pos] != '\0') {
		if (fmt[pos] != '%') {
			stream_write(NULL, p, &fmt[pos], 1);
			pos++;
			continue;
		}

		/* found a %.  now get length */
		pos++;
		length = extract_int(fmt, &pos);
                length = length;
		/* find formatting code and select formatting */
		switch (fmt[pos]) {
			case '%':
				stream_write(NULL, p, &fmt[pos], 1);
				break;
			case 'c':
			    {
				char c = (char)va_arg(args, int);

				stream_write(NULL, p, &c, 1);
				break;
			    }
			case 's':
			    {
				char *arg = va_arg(args, char *);

				stream_write(NULL, p, arg, strlen(arg));
				break;
			    }
			case 'd': case 'x': /* XXX for now! */
			    {
				int val = va_arg(args, int);
				unsigned int digit_pos;
				char *digits;

				digits = malloc(MAX_DIGITS);
				digit_pos = render_int(digits, MAX_DIGITS, val);
				stream_write(NULL, p, (digits + digit_pos), (MAX_DIGITS - digit_pos));
				free(digits);

				break;
			    }
			default:
				assert("undefined formatting code" == NULL);
				break;
		}

		pos++;
	}
}
