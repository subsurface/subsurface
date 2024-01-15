// SPDX-License-Identifier: GPL-2.0
#include "format.h"
#include "membuffer.h"

QString qasprintf_loc(const char *cformat, ...)
{
	va_list ap;
	va_start(ap, cformat);
	QString res = vqasprintf_loc(cformat, ap);
	va_end(ap);
	return res;
}

struct vasprintf_flags {
	bool alternate_form : 1;	// TODO: unsupported
	bool zero : 1;
	bool left : 1;
	bool space : 1;
	bool sign : 1;
	bool thousands : 1;		// ignored
};

enum length_modifier_t {
	LM_NONE,
	LM_CHAR,
	LM_SHORT,
	LM_LONG,
	LM_LONGLONG,
	LM_LONGDOUBLE,
	LM_INTMAX,
	LM_SIZET,
	LM_PTRDIFF
};

// Helper function to insert '+' or ' ' after last space
static QString insert_sign(const QString &s, char sign)
{
	// For space we can take a shortcut: insert in front
	if (sign == ' ')
		return sign + s;
	int size = s.size();
	int pos;
	for (pos = 0; pos < size && s[pos].isSpace(); ++pos)
		;	// Pass
	return s.left(pos) + '+' + s.mid(pos);
}

static QString fmt_string(const QString &s, vasprintf_flags flags, int field_width, int precision)
{
	int size = s.size();
	if (precision >= 0 && size > precision)
		return s.left(precision);
	return flags.left ? s.leftJustified(field_width) :
			    s.rightJustified(field_width);
}

// Formatting of integers and doubles using Qt's localized functions.
// The code is somewhat complex because Qt doesn't support all stdio
// format options, notably '+' and ' '.
// TODO: Since this is a templated function, remove common code
template <typename T>
static QString fmt_int(T i, vasprintf_flags flags, int field_width, int precision, int base)
{
	// If precision is given, things are a bit different: we have to pad with zero *and* space.
	// Therefore, treat this case separately.
	if (precision > 1) {
		// For negative numbers, increase precision by one, so that we get
		// the correct number of printed digits
		if (i < 0)
			++precision;
		QChar fillChar = '0';
		QString res = QStringLiteral("%L1").arg(i, precision, base, fillChar);
		if (i >= 0 && flags.space)
			res = ' ' + res;
		else if (i >= 0 && flags.sign)
			res = '+' + res;
		return fmt_string(res, flags, field_width, -1);
	}

	// If we have to prepend a '+' or a space character, remove that from the field width
	char sign = 0;
	if (i >= 0 && (flags.space || flags.sign) && field_width > 0) {
		sign = flags.sign ? '+' : ' ';
		--field_width;
	}
	if (flags.left)
		field_width = -field_width;
	QChar fillChar = flags.zero && !flags.left ? '0' : ' ';
	QString res = QStringLiteral("%L1").arg(i, field_width, base, fillChar);
	return sign ? insert_sign(res, sign) : res;
}

static QString fmt_float(double d, char type, vasprintf_flags flags, int field_width, int precision)
{
	// If we have to prepend a '+' or a space character, remove that from the field width
	char sign = 0;
	if (d >= 0.0 && (flags.space || flags.sign) && field_width > 0) {
		sign = flags.sign ? '+' : ' ';
		--field_width;
	}
	if (flags.left)
		field_width = -field_width;
	QChar fillChar = flags.zero && !flags.left ? '0' : ' ';
	QString res = QStringLiteral("%L1").arg(d, field_width, type, precision, fillChar);
	return sign ? insert_sign(res, sign) : res;
}

// Helper to extract integers from C-style format strings.
// The default returned value, if no digits are found, is 0.
// A '*' means fetch from varargs as int.
static int parse_fmt_int(const char **act, va_list *ap)
{
	if (**act == '*') {
		++(*act);
		return va_arg(*ap, int);
	}
	if (!isdigit(**act))
		return 0;
	int res = 0;
	while (isdigit(**act)) {
		res = res * 10 + **act - '0';
		++(*act);
	}
	return res;
}

QString vqasprintf_loc(const char *fmt, va_list ap_in)
{
	va_list ap;
	va_copy(ap, ap_in);	// Allows us to pass as pointer to helper functions
	const char *act = fmt;
	QString ret;
	for (;;) {
		// Get all bytes up to next '%' character and add them as UTF-8
		const char *begin = act;
		while (*act && *act != '%')
			++act;
		int len = act - begin;
		if (len > 0)
			ret += QString::fromUtf8(begin, len);

		// We found either a '%' or the end of the format string
		if (!*act)
			break;
		++act;	// Jump over '%'

		if (*act == '%') {
			++act;
			ret += '%';
			continue;
		}
		// Flags
		vasprintf_flags flags = { 0 };
		for (;; ++act) {
			switch(*act) {
			case '#':
				flags.alternate_form = true;
				continue;
			case '0':
				flags.zero = true;
				continue;
			case '-':
				flags.left = true;
				continue;
			case ' ':
				flags.space = true;
				continue;
			case '+':
				flags.sign = true;
				continue;
			case '\'':
				flags.thousands = true;
				continue;
			}
			break;
		}

		// Field width
		int field_width = parse_fmt_int(&act, &ap);

		// Precision
		int precision = -1;
		if (*act == '.') {
			++act;
			precision = parse_fmt_int(&act, &ap);
		}

		// Length modifier
		enum length_modifier_t length_modifier = LM_NONE;
		switch(*act) {
		case 'h':
			++act;
			length_modifier = LM_CHAR;
			if (*act == 'h') {
				length_modifier = LM_SHORT;
				++act;
			}
			break;
		case 'l':
			++act;
			length_modifier = LM_LONG;
			if (*act == 'l') {
				length_modifier = LM_LONGLONG;
				++act;
			}
			break;
		case 'q':
			++act;
			length_modifier = LM_LONGLONG;
			break;
		case 'L':
			++act;
			length_modifier = LM_LONGDOUBLE;
			break;
		case 'j':
			++act;
			length_modifier = LM_INTMAX;
			break;
		case 'z':
		case 'Z':
			++act;
			length_modifier = LM_SIZET;
			break;
		case 't':
			++act;
			length_modifier = LM_PTRDIFF;
			break;
		}

		char type = *act++;
		// Bail out if we reached end of the format string
		if (!type)
			break;

		int base = 10;
		if (type == 'o')
			base = 8;
		else if (type == 'x' || type == 'X')
			base = 16;

		switch(type) {
		case 'd': case 'i': {
			switch(length_modifier) {
			case LM_LONG:
				ret += fmt_int(va_arg(ap, long), flags, field_width, precision, base);
				break;
			case LM_LONGLONG:
				ret += fmt_int(va_arg(ap, long long), flags, field_width, precision, base);
				break;
			case LM_INTMAX:
				ret += fmt_int(va_arg(ap, intmax_t), flags, field_width, precision, base);
				break;
			case LM_SIZET:
				ret += fmt_int(va_arg(ap, ssize_t), flags, field_width, precision, base);
				break;
			case LM_PTRDIFF:
				ret += fmt_int(va_arg(ap, ptrdiff_t), flags, field_width, precision, base);
				break;
			case LM_CHAR:
				// char is promoted to int when passed through '...'
				ret += fmt_int(static_cast<char>(va_arg(ap, int)), flags, field_width, precision, base);
				break;
			case LM_SHORT:
				// short is promoted to int when passed through '...'
				ret += fmt_int(static_cast<short>(va_arg(ap, int)), flags, field_width, precision, base);
				break;
			default:
				ret += fmt_int(va_arg(ap, int), flags, field_width, precision, base);
				break;
			}
			break;
		}
		case 'o': case 'u': case 'x': case 'X': {
			QString s;
			switch(length_modifier) {
			case LM_LONG:
				s = fmt_int(va_arg(ap, unsigned long), flags, field_width, precision, base);
				break;
			case LM_LONGLONG:
				s = fmt_int(va_arg(ap, unsigned long long), flags, field_width, precision, base);
				break;
			case LM_INTMAX:
				s = fmt_int(va_arg(ap, uintmax_t), flags, field_width, precision, base);
				break;
			case LM_SIZET:
				s = fmt_int(va_arg(ap, size_t), flags, field_width, precision, base);
				break;
			case LM_PTRDIFF:
				s = fmt_int(va_arg(ap, ptrdiff_t), flags, field_width, precision, base);
				break;
			case LM_CHAR:
				// char is promoted to int when passed through '...'
				s = fmt_int(static_cast<unsigned char>(va_arg(ap, int)), flags, field_width, precision, base);
				break;
			case LM_SHORT:
				// short is promoted to int when passed through '...'
				s = fmt_int(static_cast<unsigned short>(va_arg(ap, int)), flags, field_width, precision, base);
				break;
			default:
				s = fmt_int(va_arg(ap, unsigned int), flags, field_width, precision, base);
				break;
			}
			if (type == 'X')
				s = s.toUpper();
			ret += s;
			break;
		}
		case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': {
			// It seems that Qt is not able to format long doubles,
			// therefore we have to cast down to double.
			double f = length_modifier == LM_LONGDOUBLE ?
				static_cast<double>(va_arg(ap, long double)) :
				va_arg(ap, double);
			ret += fmt_float(f, type, flags, field_width, precision);
			break;
		}
		case 'c':
			if (length_modifier == LM_LONG) {
				// Cool, on some platforms wint_t is short, on some int.
#if WINT_MAX < UINT_MAX
				wint_t wc = static_cast<wint_t>(va_arg(ap, int));
#else
				wint_t wc = va_arg(ap, wint_t);
#endif
				ret += QChar(wc);
			} else {
				ret += static_cast<char>(va_arg(ap, int));
			}
			break;
		case 's': {
			QString s = length_modifier == LM_LONG ?
				QString::fromWCharArray(va_arg(ap, wchar_t *)) :
				QString::fromUtf8(va_arg(ap, char *));
			ret += fmt_string(s, flags, field_width, precision);
			break;
		}
		case 'p':
			ret += QString("0x%1").arg(reinterpret_cast<long long>(va_arg(ap, void *)), field_width, 16);
			break;
		}
	}
	va_end(ap);
	return ret;
}

// Put a formated string respecting the default locale into a C-style array in UTF-8 encoding.
// The only complication arises from the fact that we don't want to cut through multi-byte UTF-8 code points.
extern "C" int snprintf_loc(char *dst, size_t size, const char *cformat, ...)
{
	va_list ap;
	va_start(ap, cformat);
	int res = vsnprintf_loc(dst, size, cformat, ap);
	va_end(ap);
	return res;
}

extern "C" int vsnprintf_loc(char *dst, size_t size, const char *cformat, va_list ap)
{
	QByteArray utf8 = vqasprintf_loc(cformat, ap).toUtf8();
	const char *data = utf8.constData();
	size_t utf8_size = utf8.size();
	if (size == 0)
		return utf8_size;
	if (size < utf8_size + 1) {
		memcpy(dst, data, size - 1);
		if ((data[size - 1] & 0xC0) == 0x80) {
			// We truncated a multi-byte UTF-8 encoding.
			--size;
			// Jump to last copied byte.
			if (size > 0)
				--size;
			while(size > 0 && (dst[size] & 0xC0) == 0x80)
				--size;
			dst[size] = 0;
		} else {
			dst[size - 1] = 0;
		}
	} else {
		memcpy(dst, data, utf8_size + 1); // QByteArray guarantees a trailing 0
	}
	return utf8_size;
}

int asprintf_loc(char **dst, const char *cformat, ...)
{
	va_list ap;
	va_start(ap, cformat);
	int res = vasprintf_loc(dst, cformat, ap);
	va_end(ap);
	return res;
}

int vasprintf_loc(char **dst, const char *cformat, va_list ap)
{
	QByteArray utf8 = vqasprintf_loc(cformat, ap).toUtf8();
	*dst = strdup(utf8.constData());
	return utf8.size();
}

extern "C" void put_vformat_loc(struct membuffer *b, const char *fmt, va_list args)
{
	QByteArray utf8 = vqasprintf_loc(fmt, args).toUtf8();
	const char *data = utf8.constData();
	size_t utf8_size = utf8.size();

	make_room(b, utf8_size);
	memcpy(b->buffer + b->len, data, utf8_size);
	b->len += utf8_size;
}
