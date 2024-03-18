#include "context.h"

Context::Context(const char *filename) {
    this->filename = strdup(filename);
}
Context::~Context() {
    if (filename) {
        free((void *)filename);
        filename = nullptr;
    }
    // if (fmt_ctx) {
    //     avformat_close_input(&fmt_ctx);
    //     fmt_ctx = nullptr;
    // }
}