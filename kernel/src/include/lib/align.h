#ifndef __LIB__ALIGN_H__
#define __LIB__ALIGN_H__

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)
#define divroundup(a, b) ({ (a + (b - 1)) / b; })
#define alignup(a, b) ({ divroundup(a, b) * b; })
#define aligndown(a, b) ({ (a / b) * b; })

#endif
