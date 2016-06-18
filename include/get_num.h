#ifndef GET_NUM_H
#define GET_NUM_H

#define GN_NONNEG 01  /* Value must be >= 0 */
#define GN_GT_0 02 /* Value must be > 0 */

#define GN_ANY_BASE 0100
#define GN_BASE_8 0200
#define GN_BASE_16 0400

long getLong(const char *arg, int flags, const char *name);
int getInt(const char *arg, int flags, const char *name);

#endif
