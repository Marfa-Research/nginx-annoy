#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* annoy__create();
void annoy__release(void* state);
void annoy__load(void* state, const char *filename);
void annoy__get_nns_by_item(void* state, int32_t item, size_t n, size_t search_k, int32_t *result, float *distances);
void annoy__get_item(void* state, int32_t item, double *v);
int32_t annoy__get_n_items(void* state);

#ifdef __cplusplus
}
#endif
