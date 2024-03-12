#ifndef FFPLAYER_OPTS_
#define FFPLAYER_OPTS_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

extern AVDictionary *format_opts;
extern int framedrop;

#define AV_NOSYNC_THRESHOLD 10.0

#endif