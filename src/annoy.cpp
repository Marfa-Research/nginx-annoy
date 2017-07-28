#include "annoy.h"
#include "annoylib.h"
#include "kissrandom.h"

typedef AnnoyIndex<int, double, Angular, Kiss32Random> annoy_index;

#define ANN(s) static_cast<annoy_index*>(s)

extern "C" void* annoy__create() {
    int f = 40;
	  annoy_index *t = new annoy_index(f);
    return t;
}

extern "C" void annoy__release(void* state) {
    delete ANN(state);
}

extern "C" void annoy__load(void* state, const char *filename) {
    ANN(state)->load(filename);
}

extern "C" void annoy__get_nns_by_item(void* state, int32_t item, size_t n, size_t search_k, int32_t *result, float *distances) {
    std::vector<int32_t> nns_result;

    ANN(state)->get_nns_by_item(item, n, search_k, &nns_result, NULL);

    result = (int32_t *) realloc(result, sizeof(int32_t *)*nns_result.size());
    for (uint8_t j = 0; j < nns_result.size(); j++) {
      result[j] = nns_result[j];
    }
}

extern "C" int32_t annoy__get_n_items(void* state) {
    return ANN(state)->get_n_items();
}
