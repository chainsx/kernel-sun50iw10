/*
 * linux-5.4/drivers/media/platform/sunxi-vin/utility/cfg_op.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifdef CONFIG_SENSOR_LIST_MODULE

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "cfg_op.h"

#define CFG_OK 0
#define CFG_ERR -10
#define CFG_ERR_OPEN -21
#define CFG_ERR_CREAT -22
#define CFG_ERR_RD -23
#define CFG_ERR_WR -24
#define CFG_NOT_FOUND -30
#define CFG_ERR_FMT -30
#define CFG_ERR_NOT_INIT -40

static char sct_pre = '[', sct_post = ']'; /* Section Symbol */
static char kv_iso = '='; /* isolator between key and value */
static char comment_pre = '#'; /* comment prefix */

/*
 * name:    strim_char
 * func:    trim a specific charator at front and end of a string
 * input:  char *s(string that should be trimed) char *c(specific charator)
 * output:
 * return: the string
 */
static char *strim_char(char *s, char c)
{
	int i, k, n;

	n = strlen((char *)s);
	if (n < 1)
		return s;

	for (i = 0; i < n; i++)
		if (s[i] != c)
			break;
	if (i >= n)
		return s;

	k = i;
	for (i++; i < n; i++)
		if (s[i] == c)
			break;
	if (i <= n)
		s[i] = '\0';

	return s + k;
}

/*
 * name:    get_one_line
 * func:    get one line string from buffer to line_buf
 * input:   char *buffer, int maxlen,
 * output:  char *line_buf
 * return:  the actual length of buffer which has been read
 */
static int get_one_line(char *buffer, char *line_buf, int maxlen)
{
	int i;
	char buf;

	for (i = 0; i < maxlen;) {
		buf = *buffer;

		if (buf == '\n' || buf == 0x00) /* new line */
			break;
		if (buf == '\f' || buf == 0x1A) { /* '\f':new page */
			line_buf[i++] = buf;
			break;
		}
		if (buf != '\r')
			line_buf[i++] = buf; /* ignore enter */

		buffer++;
	}
	line_buf[i++] = '\0';
	return i;
}

/*
 * name:    split_key_value
 * func:    split key and value from buf
 * input:   char *buf
 * output:  char *key, char *val
 * return:  1 --- ok
 *          0 --- blank line
 *         -1 --- no key, "=value"
 *         -2 --- only key, no '='
 */
static int split_key_value(char *buf, char *key, char *val)
{
	int i, k1, k2, n;

	n = strlen((char *)buf);
	if (n < 1)
		return 0;

	/* search for the first none space or tab value */
	for (i = 0; i < n; i++)
		if (buf[i] != ' ' && buf[i] != '\t')
			break;
	if (i >= n)
		return 0;

	/* return -1 when key is null */
	if (buf[i] == kv_iso)
		return -1;

	/* search for key */
	k1 = i;
	for (i++; i < n; i++)
		if (buf[i] == kv_iso)
			break;

	/* return -2 when only key(no isolator) */
	if (i >= n)
		return -2;

	/* search for value */
	k2 = i;
	for (i++; i < n; i++)
		if (buf[i] != ' ' && buf[i] != '\t')
			break;

	buf[k2] = '\0';

	strcpy(key, buf + k1);
	strcpy(val, buf + i);

	return 1;
}

/*
 * name:    get_one_key_value
 * func:    get a key value in the specific section from buffer
 * input:   char *buffer, void *section,void *key
 * output:  void *value
 * return:  error number
 */
static int get_one_key_value(char *buffer, void *section, void *key,
			     void *value)
{
	char buf[LINE_MAX_CHAR_NUM + 1];
	char key_ptr[MAX_NAME_LEN] = { 0 }, val_ptr[MAX_NAME_LEN] = {
	0};
	int n, ret;
	unsigned int i, buf_len;

	buf_len = strlen((char *)buffer);
	if (buf_len < 1)
		return CFG_ERR;
	for (i = 0; i < buf_len;) { /* search for section */
		ret = CFG_ERR_RD;
		n = get_one_line(buffer, buf, LINE_MAX_CHAR_NUM);
		buffer += n;
		i += n;

		ret = CFG_NOT_FOUND;
		if (n < 0)
			goto g_one_key_end; /* end of file */

		n = strlen(strim(buf));
		if (n == 0 || buf[0] == comment_pre)
			continue;  /* null line or comment line */
		ret = CFG_ERR_FMT;
		if (n > 2 && ((buf[0] == sct_pre && buf[n - 1] != sct_post)))
			goto g_one_key_end;
		if (buf[0] == sct_pre) {
			buf[n - 1] = 0x00;
			if (strcmp(buf + 1, section) == 0)
				break;  /* section found */
		}
	}

	for (; i < buf_len;) { /* search for key */
		ret = CFG_ERR_RD;
		n = get_one_line(buffer, buf, LINE_MAX_CHAR_NUM);
		buffer += n;
		i += n;

		ret = CFG_NOT_FOUND;
		if (n < 0)
			goto g_one_key_end; /* end of file */

		n = strlen(strim(buf));
		if (n == 0 || buf[0] == comment_pre)
			continue; /* null line or comment line */
		ret = CFG_NOT_FOUND;
		if (buf[0] == sct_pre)
			goto g_one_key_end;

		ret = CFG_ERR_FMT;
		if (split_key_value(buf, key_ptr, val_ptr) != 1)
			goto g_one_key_end;
		strim(key_ptr);
		if (strcmp(key_ptr, key) != 0)
			continue; /* not match */
		strcpy(value, val_ptr);
		break;
	}
	ret = CFG_OK;
g_one_key_end:
	return ret;
}

/*
 * name:    get_all_keys_value
 * func:    get all key values in the specific section from buffer
 * input:   char *buffer, void *section
 * output:  char *keys[],void *value[]
 * return:  if correct, return keys number else return error number
 */
static int get_all_keys_value(char *buffer, void *section, char *keys[],
			      void *value[])
{
	char buf[LINE_MAX_CHAR_NUM + 1];
	char key_ptr[LINE_MAX_CHAR_NUM + 1], val_ptr[LINE_MAX_CHAR_NUM + 1];
	int n, n_keys = 0, ret;
	unsigned int i, buf_len;

	buf_len = strlen((char *)buffer);
	if (buf_len < 1)
		return CFG_ERR;
	for (i = 0; i < buf_len;) { /* search for section */
		ret = CFG_ERR_RD;
		n = get_one_line(buffer, buf, LINE_MAX_CHAR_NUM);
		buffer += n;
		i += n;

		ret = CFG_NOT_FOUND;
		if (n < 0)
			goto g_all_keys_end; /* end of file */
		n = strlen(strim(buf));
		if (n == 0 || buf[0] == comment_pre)
			continue; /* null line or comment line */
		ret = CFG_ERR_FMT;
		if (n > 2 && ((buf[0] == sct_pre && buf[n - 1] != sct_post)))
			goto g_all_keys_end;
		if (buf[0] == sct_pre) {
			buf[n - 1] = 0x00;
			if (strcmp(buf + 1, section) == 0)
				break; /* section found */
		}
	}
	for (; i < buf_len;) { /* search for keys */
		ret = CFG_ERR_RD;
		n = get_one_line(buffer, buf, LINE_MAX_CHAR_NUM);
		buffer += n;
		i += n;

		if (n < 0)
			break; /* end of file */
		n = strlen(strim(buf));
		if (n == 0 || buf[0] == comment_pre)
			continue; /* null line or comment line */
		ret = CFG_NOT_FOUND;
		if (buf[0] == sct_pre)
			break; /* another section */
		ret = CFG_ERR_FMT;
		if (split_key_value(buf, key_ptr, val_ptr) != 1)
			goto g_all_keys_end;

		strim(key_ptr);
		strcpy(keys[n_keys], key_ptr);
		strim(val_ptr);
		strcpy(value[n_keys], val_ptr);

		n_keys++;
	}
	ret = n_keys;
g_all_keys_end:
	return ret;
}

/*
 * name:    get_sections
 * func:    API: get all mainkey from buffer to *sections[]
 * input:   struct file *buffer
 * output:  char *sections[]
 * return:  number of sections or error number (negative)
 */
int cfg_get_sections(char *buffer, char *sections[])
{
	char buf[LINE_MAX_CHAR_NUM + 1];
	int n, n_sections = 0, ret;
	unsigned int i, buf_len;

	buf_len = strlen((char *)buffer);
	if (buf_len < 1)
		return CFG_ERR;

	for (i = 0; i < buf_len;) { /* search for section */
		ret = CFG_ERR_RD;
		n = get_one_line(buffer, buf, LINE_MAX_CHAR_NUM);
		buffer += n;
		i += n;

		if (n < 0)
			break; /* end of file */

		n = strlen(strim(buf));
		if (n == 0 || buf[0] == comment_pre)
			continue; /* null line or comment line */
		ret = CFG_ERR_FMT;
		if (n > 2 && ((buf[0] == sct_pre && buf[n - 1] != sct_post)))
			goto get_scts_end;
		if (buf[0] == sct_pre) {
			buf[n - 1] = 0x00;
			strcpy(sections[n_sections], buf + 1);
			n_sections++;
		}
	}
	ret = n_sections;
get_scts_end:
	return ret;
}

/*
 * name:    cfg_get_one_key_value
 * func:    API: get a key value in the specific section from buffer
 * input:   char *buffer, struct cfg_mainkey *scts
 * output:  struct cfg_subkey *subkey
 * return:  error number
 */
int cfg_get_one_key_value(char *buffer, struct cfg_mainkey *scts,
			  struct cfg_subkey *subkey)
{
	int ret, len;
	char str_tmp[MAX_VALUE_LEN];
	char *endp;

	if (scts == NULL || subkey == NULL)
		return CFG_ERR_NOT_INIT;

	if (scts->cfg_flag != CFG_KEY_INIT || subkey->cfg_flag != CFG_KEY_INIT)
		return CFG_ERR_NOT_INIT;

	ret =
	    get_one_key_value(buffer, scts->name, subkey->name,
			      (void *)subkey->value.str);
	if (ret)
		return ret;

	len = strlen(subkey->value.str);
	strcpy(str_tmp, subkey->value.str);

	if (str_tmp[0] == '"' && str_tmp[len - 1] == '"') {
		subkey->type = CFG_ITEM_VALUE_TYPE_STR;
		strcpy(subkey->value.str, strim_char(subkey->value.str, '"'));
	} else if ((str_tmp[0] >= 0x30 && str_tmp[0] <= 0x39)
		   || str_tmp[0] == '-') {
		subkey->type = CFG_ITEM_VALUE_TYPE_INT;
		endp = "\0";
		subkey->value.val =
		    (int)simple_strtol(subkey->value.str, &endp, 0);
	} else {
		subkey->type = CFG_ITEM_VALUE_TYPE_INVALID;
	}
	return 0;
}

/*
 * name:    cfg_get_all_keys_value
 * func:    API: get all key values in the specific section from buffer
 * input:   char *buffer, struct cfg_mainkey *scts
 * output:  struct cfg_mainkey *scts
 * return:  error number
 */
int cfg_get_all_keys_value(char *buffer, struct cfg_mainkey *scts)
{
	int i, len;
	char str_tmp[MAX_NAME_LEN];
	char *endp;

	if (scts == NULL)
		return CFG_ERR_NOT_INIT;

	if (scts->cfg_flag != CFG_KEY_INIT)
		return CFG_ERR_NOT_INIT;

	scts->subkey_cnt =
	    get_all_keys_value(buffer, scts->name, scts->subkey_name,
			       (void *)scts->subkey_value);

	if (scts->subkey_cnt <= 0)
		return 0;

	for (i = 0; i < scts->subkey_cnt; i++) {
		len = strlen(scts->subkey[i].value.str);
		strcpy(str_tmp, scts->subkey[i].value.str);

		if (str_tmp[0] == '"' && str_tmp[len - 1] == '"') {
			scts->subkey[i].type = CFG_ITEM_VALUE_TYPE_STR;
			strcpy(scts->subkey[i].value.str,
			       strim_char(scts->subkey[i].value.str, '"'));
		} else if ((str_tmp[0] >= 0x30 && str_tmp[0] <= 0x39)
			   || str_tmp[0] == '-') {
			scts->subkey[i].type = CFG_ITEM_VALUE_TYPE_INT;
			endp = "\0";
			scts->subkey[i].value.val =
			    (int)simple_strtol(scts->subkey[i].value.str, &endp,
					       0);
		} else {
			scts->subkey[i].type = CFG_ITEM_VALUE_TYPE_INVALID;
		}
	}

	return scts->subkey_cnt;
}

/*
 * name:    cfg_mainkey_init
 * func:    API: init resource before using mainkey
 * input:   struct cfg_mainkey **mainkey
 * output:
 * return:  error number
 */
void cfg_mainkey_init(struct cfg_mainkey **mainkey, char **mainkey_name)
{
	int n;
	*mainkey = (struct cfg_mainkey *)vmalloc(sizeof(struct cfg_mainkey));
	memset(*mainkey, 0, sizeof(struct cfg_mainkey));
	*mainkey_name = (*mainkey)->name;
	for (n = 0; n < MAX_SUBKEY_NUM; n++) {
		(*mainkey)->subkey_name[n] = (*mainkey)->subkey[n].name;
		(*mainkey)->subkey_value[n] = (*mainkey)->subkey[n].value.str;
	}
	(*mainkey)->cfg_flag = CFG_KEY_INIT;
}

/*
 * name:    cfg_mainkey_release
 * func:    API: release resource after using mainkey
 * input:   struct cfg_mainkey **mainkey
 * output:
 * return:  error number
 */
void cfg_mainkey_release(struct cfg_mainkey **mainkey, char **mainkey_name)
{
	(*mainkey)->cfg_flag = CFG_KEY_RELEASE;
	if (*mainkey)
		vfree(*mainkey);
}

/*
 * name:    cfg_section_init
 * func:    API: init resource before using cfg_section
 * input:   struct cfg_section **cfg_sct
 * output:
 * return:  error number
 */
void cfg_section_init(struct cfg_section **cfg_sct)
{
	int i;

	*cfg_sct =
	    (struct cfg_section *)kzalloc(sizeof(struct cfg_section),
					  GFP_KERNEL);
	for (i = 0; i < MAX_MAINKEY_NUM; i++)
		cfg_mainkey_init(&(*cfg_sct)->mainkey[i],
				 &(*cfg_sct)->mainkey_name[i]);
	(*cfg_sct)->cfg_flag = CFG_KEY_INIT;
}

/*
 * name:    cfg_section_release
 * func:    API: release resource after using cfg_section
 * input:   struct cfg_section **cfg_sct
 * output:
 * return:  error number
 */
void cfg_section_release(struct cfg_section **cfg_sct)
{
	int i;

	(*cfg_sct)->cfg_flag = CFG_KEY_RELEASE;
	for (i = 0; i < MAX_MAINKEY_NUM; i++)
		cfg_mainkey_release(&(*cfg_sct)->mainkey[i],
				    &(*cfg_sct)->mainkey_name[i]);
	if (*cfg_sct)
		kfree(*cfg_sct);
}

struct file *cfg_open_file(char *file_path)
{
	struct file *fp;

	fp = filp_open(file_path, O_RDWR | O_APPEND | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		pr_info("[vin_warn]open %s failed!, ERR NO is %ld.\n", file_path,
		       (long)fp);
	}
	return fp;
}
int cfg_close_file(struct file *fp)
{
	if (IS_ERR(fp)) {
		pr_info("[vin_warn]colse file failed,fp is invaild!\n");
		return -1;
	} else {
		filp_close(fp, NULL);
		return 0;
	}
}

/*
 * name:    cfg_read_file
 * func:    API: read from file to buf
 * input:   char *file_path,size_t len
 * output:  char *buf
 * return:  the actual length has been read
 */
int cfg_read_file(char *file_path, char *buf, size_t len)
{
	struct file *fp;
	mm_segment_t old_fs;
	loff_t pos = 0;
	int buf_len;

	fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_info("[vin_warn]open file failed!\n");
		return -EFAULT;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	buf_len = vfs_read(fp, buf, len, &pos);
	set_fs(old_fs);

	if (fp)
		filp_close(fp, NULL);

	if (buf_len < 0)
		return -1;

	return buf_len;
}

int cfg_write_file(struct file *fp, char *buf, size_t len)
{
	mm_segment_t old_fs;
	loff_t pos = 0;
	int buf_len;

	if (IS_ERR_OR_NULL(fp)) {
		pr_info("cfg write file error, fp is null!");
		return -1;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	buf_len = vfs_write(fp, buf, len, &pos);
	set_fs(old_fs);

	if (buf_len < 0)
		return -1;
	if (buf_len != len)
		pr_info("buf_len = %x, len = %pa\n", buf_len, &len);
	return buf_len;
}

/*
 * name:    cfg_read_ini
 * func:    API: read from file, parse mainkey and sunkey value,save to cfg_setction
 * input:   char *file_path
 * output:  struct cfg_section **cfg_section
 * return:  error number
 */
int cfg_read_ini(char *file_path, struct cfg_section **cfg_section)
{
	unsigned int i;
	struct cfg_section *cfg_sct = *(cfg_section);
	char *buf;
	int buf_len, ret = 0;

	buf = (char *)vmalloc(INI_MAX_CHAR_NUM);
	memset(buf, 0, INI_MAX_CHAR_NUM);

	buf_len = cfg_read_file(file_path, buf, INI_MAX_CHAR_NUM);
	if (buf_len < 0) {
		ret = -1;
		goto rd_ini_end;
	}
	buf[buf_len] = '\0';

	/* parse file */
	cfg_sct->mainkey_cnt = cfg_get_sections(buf, cfg_sct->mainkey_name);
	for (i = 0; i < cfg_sct->mainkey_cnt; i++) {
		cfg_sct->mainkey[i]->subkey_cnt =
		    cfg_get_all_keys_value(buf, cfg_sct->mainkey[i]);
	}

rd_ini_end:
	if (buf)
		vfree(buf);

	return ret;
}

/*
 * name:    cfg_get_ini_item
 * func:    API: read from cfg_setction, parse mainkey and sunkey value,save to cfg_setction
 * input:   struct cfg_section *cfg_section, char *main, char *sub
 * output:  struct cfg_subkey *subkey
 * return:  error number
 */
int cfg_get_one_subkey(struct cfg_section *cs, char *main, char *sub,
		       struct cfg_subkey *subkey)
{
	int i, j, ret;

	for (i = 0; i < cs->mainkey_cnt; i++) {
		if (strcmp(cs->mainkey_name[i], main) != 0)
			continue;
		for (j = 0; j < cs->mainkey[i]->subkey_cnt; j++) {
			if (strcmp(cs->mainkey[i]->subkey_name[j], sub) == 0) {
				ret = cs->mainkey[i]->subkey[j].type;
				memcpy(subkey, &cs->mainkey[i]->subkey[j],
				       sizeof(struct cfg_subkey));
				subkey = &cs->mainkey[i]->subkey[j];
				return ret;
			}
		}
	}
	return -1;
}
#endif

