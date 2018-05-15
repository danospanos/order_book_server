/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#ifndef JSON_FUNCS_H
#define JSON_FUNCS_H

#include "jsmn.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/**
 * @uthor Serge Zaitsev
 * (c) -  http://zserge.com/jsmn.html
 */
int jsoneq(const char *json, jsmntok_t *tok, const char *s);
/**
 * @brief Checks if string pass to json string and token currently looking at
 */
void parse_json_add_to_strings(char *message_from_clnt, char **message, char **orderId, char **price, char **quantity, char **side);
/**
 * @brief strndup() mallocated some memory that is needed to be freed
 */
void free_all_data_from_heap(char *message, char *orderId, char *price, char *quantity, char *side);

#endif
