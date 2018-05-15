/**
 * @author     Daniel Stasek
 * @date       June 24, 2016
 * @version    1.0
 */
#include "json_funcs.h"

int jsoneq(const char *json, jsmntok_t *tok, const char *s){
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;

}

void parse_json_add_to_strings(char *message_from_clnt, char **message, char **orderId, char **price, char **quantity, char **side){
  jsmntok_t t[12];
  int i, r;
  jsmn_parser p;

	jsmn_init(&p);
  r = jsmn_parse(&p, message_from_clnt, strlen(message_from_clnt), t, sizeof(t)/sizeof(t[0]));

  if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
	}
  for (i = 1; i < r; i++) {
		if (jsoneq(message_from_clnt, &t[i], "side") == 0) {
			*side = strndup((message_from_clnt + t[i+1].start), (t[i+1].end-t[i+1].start));
			i++;
		} else if (jsoneq(message_from_clnt, &t[i], "message") == 0) {
      *message = strndup((message_from_clnt + t[i+1].start), (t[i+1].end-t[i+1].start));
      i++;
		} else if (jsoneq(message_from_clnt, &t[i], "orderId") == 0) {
			*orderId = strndup((message_from_clnt + t[i+1].start), (t[i+1].end-t[i+1].start));
			i++;
		} else if (jsoneq(message_from_clnt, &t[i], "price") == 0) {
			*price = strndup((message_from_clnt + t[i+1].start), (t[i+1].end-t[i+1].start));
			i++;
		} else if (jsoneq(message_from_clnt, &t[i], "quantity") == 0) {
			*quantity = strndup((message_from_clnt + t[i+1].start), (t[i+1].end-t[i+1].start));
			i++;
		} else {
			printf("Unexpected key: %.*s\n", t[i].end-t[i].start,
					message_from_clnt + t[i].start);
		}
	}
}

void free_all_data_from_heap(char *message, char *orderId, char *price, char *quantity, char *side){
  free(message);
  free(orderId);
  free(price);
  free(quantity);
  free(side);
}
