/* SPDX-License-Identifier: MIT */
#include "esa_rules.h"
#include "esa_rules.h"
#include "esa.h"
#include "esa.h"
#include <cstdlib>
#include <cstring>

int main()
{
    Esa esa;
    char *stem = nullptr;
    if (ESA_DEFAULT_RULE_COUNT == 0 || esaInitDefault(&esa) != ESA_OK) {
        return EXIT_FAILURE;
    }
    EsaStatus status = esaStemAlloc(&esa, "Stemming", 8, &stem, nullptr);
    int result = status == ESA_OK && std::strcmp(stem, "stem") == 0
                     ? EXIT_SUCCESS : EXIT_FAILURE;
    std::free(stem);
    esaFree(&esa);
    return result;
}
