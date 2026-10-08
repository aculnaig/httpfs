/*
    httpfs - Sitemap streaming XML parser implementation

    Copyright (C) 2026 Free Software Foundation, Inc.
    Written by Gianluca Cannata <gcannata23@gmail.com>
    This file is part of the GNU Hurd.

    The GNU Hurd is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The GNU Hurd is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "sitemap_xml.h"

#include <libxml/xmlreader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

time_t sitemap_parse_iso8601(const char *date_str)
{
    if (!date_str || !*date_str) {
        return (time_t)0;
    }

    int year = 0, mon = 0, mday = 0;
    int hour = 0, min = 0, sec = 0;
    char tz_sign = '\0';
    int tz_hour = 0, tz_min = 0;

    /* Check if time component 'T' is present */
    const char *t_ptr = strchr(date_str, 'T');
    if (!t_ptr) {
        t_ptr = strchr(date_str, 't');
    }

    if (!t_ptr) {
        /* Format: YYYY-MM-DD */
        if (sscanf(date_str, "%d-%d-%d", &year, &mon, &mday) != 3) {
            return (time_t)0;
        }
    } else {
        /* Format: YYYY-MM-DDThh:mm:ss with optional timezone */
        int parsed = sscanf(date_str, "%d-%d-%d%*1[Tt]%d:%d:%d%c%d:%d",
                            &year, &mon, &mday, &hour, &min, &sec,
                            &tz_sign, &tz_hour, &tz_min);
        if (parsed < 3) {
            return (time_t)0;
        }
    }

    if (year < 1970 || mon < 1 || mon > 12 || mday < 1 || mday > 31) {
        return (time_t)0;
    }

    struct tm tm_val;
    memset(&tm_val, 0, sizeof(struct tm));
    tm_val.tm_year = year - 1900;
    tm_val.tm_mon = mon - 1;
    tm_val.tm_mday = mday;
    tm_val.tm_hour = hour;
    tm_val.tm_min = min;
    tm_val.tm_sec = sec;
    tm_val.tm_isdst = 0;

    time_t epoch = timegm(&tm_val);
    if (epoch == (time_t)-1) {
        return (time_t)0;
    }

    /* Apply timezone offset if present (+HH:MM or -HH:MM) */
    if (tz_sign == '+') {
        epoch -= (tz_hour * 3600 + tz_min * 60);
    } else if (tz_sign == '-') {
        epoch += (tz_hour * 3600 + tz_min * 60);
    }

    return epoch;
}

char *sitemap_extract_path_from_url(const char *url_or_path)
{
    if (!url_or_path || !*url_or_path) {
        return strdup("/");
    }

    const char *scheme_sep = strstr(url_or_path, "://");
    if (scheme_sep) {
        /* Advance past "://" */
        const char *after_scheme = scheme_sep + 3;
        const char *first_slash = strchr(after_scheme, '/');
        if (first_slash) {
            return strdup(first_slash);
        }
        return strdup("/");
    }

    if (url_or_path[0] == '/') {
        return strdup(url_or_path);
    }

    /* Relative path without leading slash */
    size_t len = strlen(url_or_path) + 2;
    char *res = malloc(len);
    if (!res) {
        return NULL;
    }
    snprintf(res, len, "/%s", url_or_path);
    return res;
}

void sitemap_index_list_free(struct sitemap_index_entry *list)
{
    while (list) {
        struct sitemap_index_entry *next = list->next;
        if (list->loc) {
            free(list->loc);
        }
        free(list);
        list = next;
    }
}

static void append_index_entry(struct sitemap_index_entry **list_head,
                               const char *loc,
                               time_t lastmod)
{
    if (!list_head || !loc) {
        return;
    }

    struct sitemap_index_entry *entry = malloc(sizeof(struct sitemap_index_entry));
    if (!entry) {
        return;
    }

    entry->loc = strdup(loc);
    entry->last_modified = lastmod;
    entry->next = NULL;

    if (*list_head == NULL) {
        *list_head = entry;
    } else {
        struct sitemap_index_entry *curr = *list_head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = entry;
    }
}

int sitemap_parse_xml_buffer(struct sitemap_node *root,
                            const char *xml_data,
                            size_t size,
                            struct sitemap_index_entry **out_sub_sitemaps)
{
    if (!root || !xml_data || size == 0) {
        return EINVAL;
    }

    xmlTextReaderPtr reader = xmlReaderForMemory(xml_data, (int)size, NULL, NULL,
                                                XML_PARSE_NOENT | XML_PARSE_NONET);
    if (!reader) {
        return EINVAL;
    }

    bool in_url = false;
    bool in_sitemap = false;
    char *current_loc = NULL;
    char *current_lastmod = NULL;
    int ret;
    int err = 0;

    while ((ret = xmlTextReaderRead(reader)) == 1) {
        int node_type = xmlTextReaderNodeType(reader);
        const xmlChar *name = xmlTextReaderConstLocalName(reader);
        if (!name) {
            continue;
        }

        if (node_type == XML_READER_TYPE_ELEMENT) {
            if (strcmp((const char *)name, "url") == 0) {
                in_url = true;
                free(current_loc);
                current_loc = NULL;
                free(current_lastmod);
                current_lastmod = NULL;
            } else if (strcmp((const char *)name, "sitemap") == 0) {
                in_sitemap = true;
                free(current_loc);
                current_loc = NULL;
                free(current_lastmod);
                current_lastmod = NULL;
            } else if (in_url || in_sitemap) {
                if (strcmp((const char *)name, "loc") == 0) {
                    xmlChar *val = xmlTextReaderReadString(reader);
                    if (val) {
                        free(current_loc);
                        current_loc = strdup((const char *)val);
                        xmlFree(val);
                    }
                } else if (strcmp((const char *)name, "lastmod") == 0) {
                    xmlChar *val = xmlTextReaderReadString(reader);
                    if (val) {
                        free(current_lastmod);
                        current_lastmod = strdup((const char *)val);
                        xmlFree(val);
                    }
                }
            }
        } else if (node_type == XML_READER_TYPE_END_ELEMENT) {
            if (strcmp((const char *)name, "url") == 0) {
                if (current_loc) {
                    char *path = sitemap_extract_path_from_url(current_loc);
                    if (path) {
                        size_t path_len = strlen(path);
                        bool is_dir = (path_len > 1 && path[path_len - 1] == '/');
                        struct sitemap_node *node = sitemap_insert_path(root, path, is_dir);
                        if (node) {
                            pthread_mutex_lock(&node->lock);
                            if (!node->url) {
                                node->url = strdup(current_loc);
                            }
                            if (current_lastmod) {
                                node->last_modified = sitemap_parse_iso8601(current_lastmod);
                            }
                            node->state = NODE_STATE_PENDING;
                            pthread_mutex_unlock(&node->lock);
                        }
                        free(path);
                    }
                    free(current_loc);
                    current_loc = NULL;
                }
                free(current_lastmod);
                current_lastmod = NULL;
                in_url = false;
            } else if (strcmp((const char *)name, "sitemap") == 0) {
                if (current_loc && out_sub_sitemaps) {
                    time_t lastmod = current_lastmod ? sitemap_parse_iso8601(current_lastmod) : (time_t)0;
                    append_index_entry(out_sub_sitemaps, current_loc, lastmod);
                    free(current_loc);
                    current_loc = NULL;
                }
                free(current_lastmod);
                current_lastmod = NULL;
                in_sitemap = false;
            }
        }
    }

    if (ret < 0) {
        err = EINVAL;
    }

    if (current_loc) {
        free(current_loc);
    }
    if (current_lastmod) {
        free(current_lastmod);
    }

    xmlFreeTextReader(reader);
    return err;
}
