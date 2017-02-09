
/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <libss.h>
#include <libsf.h>

static inline sshot int
sf_cmpstring(char *a, int asz, char *b, int bsz, void *arg ssunused)
{
	int size = (asz < bsz) ? asz : bsz;
	int rc = memcmp(a, b, size);
	if (ssunlikely(rc == 0)) {
		if (sslikely(asz == bsz))
			return 0;
		return (asz < bsz) ? -1 : 1;
	}
	return rc > 0 ? 1 : -1;
}

static inline sshot int
sf_cmpstring_reverse(char *a, int asz, char *b, int bsz, void *arg ssunused)
{
	int size = (asz < bsz) ? asz : bsz;
	int rc = memcmp(a, b, size);
	if (ssunlikely(rc == 0)) {
		if (sslikely(asz == bsz))
			return 0;
		//TODO reverse for compare with prefix?
		return (asz < bsz) ? 1 : -1;
	}
	return rc > 0 ? -1 : 1;
}

static inline sshot int
sf_cmpu8(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	uint8_t av = *(uint8_t*)a;
	uint8_t bv = *(uint8_t*)b;
	if (av == bv)
		return 0;
	return (av > bv) ? 1 : -1;
}

static inline sshot int
sf_cmpu8_reverse(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	uint8_t av = *(uint8_t*)a;
	uint8_t bv = *(uint8_t*)b;
	if (av == bv)
		return 0;
	return (av > bv) ? -1 : 1;
}

static inline sshot int
sf_cmpu16(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	uint16_t av = *(uint16_t*)a;
	uint16_t bv = *(uint16_t*)b;
	if (av == bv)
		return 0;
	return (av > bv) ? 1 : -1;
}

static inline sshot int
sf_cmpu16_reverse(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	uint16_t av = *(uint16_t*)a;
	uint16_t bv = *(uint16_t*)b;
	if (av == bv)
		return 0;
	return (av > bv) ? -1 : 1;
}

static inline sshot int
sf_cmpu32(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	uint32_t av = sscastu32(a);
	uint32_t bv = sscastu32(b);
	if (av == bv)
		return 0;
	return (av > bv) ? 1 : -1;
}

static inline sshot int
sf_cmpu32_reverse(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	uint32_t av = sscastu32(a);
	uint32_t bv = sscastu32(b);
	if (av == bv)
		return 0;
	return (av > bv) ? -1 : 1;
}

static inline sshot int
sf_cmpu64(char *a, int asz ssunused, char *b, int bsz ssunused,
              void *arg ssunused)
{
	uint64_t av = sscastu64(a);
	uint64_t bv = sscastu64(b);
	if (av == bv)
		return 0;
	return (av > bv) ? 1 : -1;
}

static inline sshot int
sf_cmpu64_reverse(char *a, int asz ssunused, char *b, int bsz ssunused,
              void *arg ssunused)
{
	uint64_t av = sscastu64(a);
	uint64_t bv = sscastu64(b);
	if (av == bv)
		return 0;
	return (av > bv) ? -1 : 1;
}

sshot int
sf_compare(sfscheme *s, char *a, char *b)
{
	sffield **part = s->keys;
	sffield **last = part + s->keys_count;
	int rc;
	while (part < last) {
		sffield *key = *part;
		uint32_t a_fieldsize;
		char *a_field = sf_fieldptr(s, key, a, &a_fieldsize);
		uint32_t b_fieldsize;
		char *b_field = sf_fieldptr(s, key, b, &b_fieldsize);
		rc = key->cmp(a_field, a_fieldsize, b_field, b_fieldsize, NULL);
		if (rc != 0)
			return rc;
		part++;
	}
	return 0;
}

sshot int
sf_compareprefix(sfscheme *s, char *prefix, uint32_t prefixsize, char *key)
{
	uint32_t keysize;
	key = sf_field(s, 0, key, &keysize);
	if (keysize < prefixsize)
		return 0;
	return (memcmp(prefix, key, prefixsize) == 0) ? 1 : 0;
}

void sf_schemeinit(sfscheme *s)
{
	s->fields = NULL;
	s->fields_count = 0;
	s->keys = NULL;
	s->keys_count = 0;
	s->var_offset = 0;
	s->offset_expire = 0;
	s->offset_lsn = 0;
	s->offset_flags = 0;
	s->var_count  = 0;
	s->cmp = NULL;
	s->cmparg = NULL;
	s->has_lsn = 0;
	s->has_flags = 0;
	s->has_timestamp = 0;
	s->has_expire = 0;
}

void sf_schemefree(sfscheme *s, ssa *a)
{
	if (s->fields) {
		int i = 0;
		while (i < s->fields_count) {
			sf_fieldfree(s->fields[i], a);
			i++;
		}
		ss_free(a, s->fields);
		s->fields = NULL;
	}
	if (s->keys) {
		ss_free(a, s->keys);
		s->keys = NULL;
	}
}

int sf_schemeadd(sfscheme *s, ssa *a, sffield *f)
{
	int size = sizeof(sffield*) * (s->fields_count + 1);
	sffield **fields = ss_malloc(a, size);
	if (ssunlikely(fields == NULL))
		return -1;
	memcpy(fields, s->fields, size - sizeof(sffield*));
	fields[s->fields_count] = f;
	f->position = s->fields_count;
	f->position_key = -1;
	if (s->fields)
		ss_free(a, s->fields);
	s->fields = fields;
	s->fields_count++;
	return 0;
}

static inline int
sf_schemeset(sfscheme *s, sffield *f, char *opt)
{
	(void)s;
	if (strcmp(opt, "string") == 0) {
		f->type = SS_STRING;
		f->fixed_size = 0;
		f->cmp = sf_cmpstring;
	} else
	if (strcmp(opt, "string_rev") == 0) {
		f->type = SS_STRINGREV;
		f->fixed_size = 0;
		f->cmp = sf_cmpstring_reverse;
	} else
	if (strcmp(opt, "u8") == 0) {
		f->type = SS_U8;
		f->fixed_size = sizeof(uint8_t);
		f->cmp = sf_cmpu8;
	} else
	if (strcmp(opt, "u8_rev") == 0) {
		f->type = SS_U8REV;
		f->fixed_size = sizeof(uint8_t);
		f->cmp = sf_cmpu8_reverse;
	} else
	if (strcmp(opt, "u16") == 0) {
		f->type = SS_U16;
		f->fixed_size = sizeof(uint16_t);
		f->cmp = sf_cmpu16;
	} else
	if (strcmp(opt, "u16_rev") == 0) {
		f->type = SS_U16REV;
		f->fixed_size = sizeof(uint16_t);
		f->cmp = sf_cmpu16_reverse;
	} else
	if (strcmp(opt, "u32") == 0) {
		f->type = SS_U32;
		f->fixed_size = sizeof(uint32_t);
		f->cmp = sf_cmpu32;
	} else
	if (strcmp(opt, "u32_rev") == 0) {
		f->type = SS_U32REV;
		f->fixed_size = sizeof(uint32_t);
		f->cmp = sf_cmpu32_reverse;
	} else
	if (strcmp(opt, "u64") == 0) {
		f->type = SS_U64;
		f->fixed_size = sizeof(uint64_t);
		f->cmp = sf_cmpu64;
	} else
	if (strcmp(opt, "u64_rev") == 0) {
		f->type = SS_U64REV;
		f->fixed_size = sizeof(uint64_t);
		f->cmp = sf_cmpu64_reverse;
	} else
	if (strncmp(opt, "key", 3) == 0) {
		char *p = opt + 3;
		if (ssunlikely(*p != '('))
			return -1;
		p++;
		if (ssunlikely(! isdigit(*p)))
			return -1;
		int v = 0;
		while (isdigit(*p)) {
			v = (v * 10) + *p - '0';
			p++;
		}
		if (ssunlikely(*p != ')'))
			return -1;
		p++;
		f->position_key = v;
		f->key = 1;
	} else
	if (strncmp(opt, "lsn", 3) == 0) {
		f->lsn = 1;
	} else
	if (strncmp(opt, "flags", 5) == 0) {
		f->flags = 1;
	} else
	if (strncmp(opt, "timestamp", 9) == 0) {
		f->timestamp = 1;
	} else
	if (strncmp(opt, "expire", 6) == 0) {
		f->expire = 1;
	} else {
		return -1;
	}
	return 0;
}

int
sf_schemevalidate(sfscheme *s, ssa *a)
{
	/* validate fields */
	if (s->fields_count == 0) {
		return -1;
	}

	/* add meta fields */
	int rc;

	/* flags */
	sffield *meta_flags = sf_fieldnew(a, "_flags");
	if (ssunlikely(meta_flags == NULL))
		return -1;
	rc = sf_fieldoptions(meta_flags, a, "u8,flags");
	if (ssunlikely(rc == -1)) {
		sf_fieldfree(meta_flags, a);
		return -1;
	}
	rc = sf_schemeadd(s, a, meta_flags);
	if (ssunlikely(rc == -1)) {
		sf_fieldfree(meta_flags, a);
		return -1;
	}

	/* lsn */
	sffield *meta_lsn = sf_fieldnew(a, "_lsn");
	if (ssunlikely(meta_lsn == NULL))
		return -1;
	rc = sf_fieldoptions(meta_lsn, a, "u64,lsn");
	if (ssunlikely(rc == -1)) {
		sf_fieldfree(meta_lsn, a);
		return -1;
	}
	rc = sf_schemeadd(s, a, meta_lsn);
	if (ssunlikely(rc == -1)) {
		sf_fieldfree(meta_lsn, a);
		return -1;
	}

	int fixed_offset = 0;
	int fixed_pos = 0;
	int i = 0;
	while (i < s->fields_count)
	{
		/* validate and apply field options */
		sffield *f = s->fields[i];
		if (f->options == NULL) {
			return -1;
		}
		/* set user compare function */
		if (s->cmp) {
			f->cmp = s->cmp;
		}
		char opts[256];
		snprintf(opts, sizeof(opts), "%s", f->options);
		char *p;
		for (p = strtok(opts, " ,"); p;
		     p = strtok(NULL, " ,"))
		{
			rc = sf_schemeset(s, f, p);
			if (ssunlikely(rc == -1))
				return -1;
		}
		/* validate auto modifiers */
		if (f->timestamp) {
			if (f->type != SS_U32)
				return -1;
			s->has_timestamp = 1;
		}
		if (f->expire) {
			if (! f->timestamp)
				return -1;
			if (s->has_expire)
				return -1;
			s->has_expire = 1;
		}
		/* meta fields */

		/* flags */
		if (f->flags) {
			if (f->type != SS_U8)
				return -1;
			if (s->has_flags)
				return -1;
			s->has_flags = 1;
		}
		/* lsn */
		if (f->lsn) {
			if (f->type != SS_U64)
				return -1;
			if (s->has_lsn)
				return -1;
			s->has_lsn = 1;
		}

		/* calculate offset and position for fixed
		 * size types */
		if (f->fixed_size > 0) {
			f->position_ref = fixed_pos;
			fixed_pos++;
			f->fixed_offset = fixed_offset;
			fixed_offset += f->fixed_size;
			if (f->expire)
				s->offset_expire = f->fixed_offset;
			else
			if (f->lsn)
				s->offset_lsn = f->fixed_offset;
			else
			if (f->flags)
				s->offset_flags = f->fixed_offset;
		} else {
			s->var_count++;
		}
		if (f->key)
			s->keys_count++;
		i++;
	}
	s->var_offset = fixed_offset;

	/* validate keys */
	if (ssunlikely(s->keys_count == 0))
		return -1;
	int size = sizeof(sffield*) * s->keys_count;
	s->keys = ss_malloc(a, size);
	if (ssunlikely(s->keys == NULL))
		return -1;
	memset(s->keys, 0, size);
	int pos_var = 0;
	i = 0;
	while (i < s->fields_count) {
		sffield *f = s->fields[i];
		if (f->key) {
			if (ssunlikely(f->position_key < 0))
				return -1;
			if (ssunlikely(f->position_key >= s->fields_count))
				return -1;
			if (ssunlikely(f->position_key >= s->keys_count))
				return -1;
			if (ssunlikely(s->keys[f->position_key] != NULL))
				return -1;
			s->keys[f->position_key] = f;
		}
		if (f->fixed_size == 0)
			f->position_ref = pos_var++;
		i++;
	}
	i = 0;
	while (i < s->keys_count) {
		sffield *f = s->keys[i];
		if (f == NULL)
			return -1;
		i++;
	}
	return 0;
}

int sf_schemesave(sfscheme *s, ssa *a, ssbuf *buf)
{
	/* fields count */
	uint32_t v = s->fields_count;
	if (s->has_lsn)
		v--;
	if (s->has_flags)
		v--;
	int rc = ss_bufadd(buf, a, &v, sizeof(uint32_t));
	if (ssunlikely(rc == -1))
		return -1;
	int fields_count = v;
	int i = 0;
	while (i < fields_count) {
		sffield *field = s->fields[i];
		assert(field->lsn == 0);
		assert(field->flags == 0);
		/* name */
		v = strlen(field->name) + 1;
		rc = ss_bufensure(buf, a, sizeof(uint32_t) + v);
		if (ssunlikely(rc == -1))
			goto error;
		memcpy(buf->p, &v, sizeof(v));
		ss_bufadvance(buf, sizeof(uint32_t));
		memcpy(buf->p, field->name, v);
		ss_bufadvance(buf, v);
		/* options */
		v = strlen(field->options) + 1;
		rc = ss_bufensure(buf, a, sizeof(uint32_t) + v);
		if (ssunlikely(rc == -1))
			goto error;
		memcpy(buf->p, &v, sizeof(v));
		ss_bufadvance(buf, sizeof(uint32_t));
		memcpy(buf->p, field->options, v);
		ss_bufadvance(buf, v);
		i++;
	}
	return 0;
error:
	ss_buffree(buf, a);
	return -1;
}

int sf_schemeload(sfscheme *s, ssa *a, char *buf, int size ssunused)
{
	/* count */
	char *p = buf;
	uint32_t v = sscastu32(p);
	p += sizeof(uint32_t);
	int count = v;
	int i = 0;
	while (i < count) {
		/* name */
		v = sscastu32(p);
		p += sizeof(uint32_t);
		sffield *field = sf_fieldnew(a, p);
		if (ssunlikely(field == NULL))
			goto error;
		p += v;
		/* options */
		v = sscastu32(p);
		p += sizeof(uint32_t);
		int rc = sf_fieldoptions(field, a, p);
		if (ssunlikely(rc == -1)) {
			sf_fieldfree(field, a);
			goto error;
		}
		rc = sf_schemeadd(s, a, field);
		if (ssunlikely(rc == -1)) {
			sf_fieldfree(field, a);
			goto error;
		}
		p += v;
		i++;
	}
	return 0;
error:
	sf_schemefree(s, a);
	return -1;
}

sffield*
sf_schemefind(sfscheme *s, char *name)
{
	int i;
	for (i = 0; i < s->fields_count; i++)
		if (strcmp(s->fields[i]->name, name) == 0)
			return s->fields[i];
	return NULL;
}
