#ifndef _MAXMINCLAMP_H_
#define _MAXMINCLAMP_H_

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__clang__)
#define __cmp_op_min <
#define __cmp_op_max >
#define __cmp(op, x, y) ((x) __cmp_op_##op (y) ? (x) : (y))
#define __careful_cmp(op, x, y) __extension__({       \
            __auto_type _x__ = (x);                   \
            __auto_type _y__ = (y);                   \
            (void) (&_x__ == &_y__);                  \
            __cmp(op, _x__, _y__); })

#define MAX(x,y) __careful_cmp(max, x, y)
#define MIN(x,y) __careful_cmp(min, x, y)

#define CLAMP(x, xmin, xmax) __extension__({                                   \
            __auto_type _x__ = (x);                                            \
            __typeof__(_x__) _xmin__ = (xmin);                                 \
            __typeof__(_x__) _xmax__ = (xmax);                                 \
            _x__ <= _xmin__ ? _xmin__ : _x__ >= _xmax__ ? _xmax__ : _x__; })

#else
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define CLAMP(x, xmin, xmax) (MAX((xim), (MIN((x), (xmax)))))
#endif

#define SWAP(x,y) do {                                                         \
        __typeof__(x) _tmp__ = (x);                                            \
        (void) (&x == &y);                                                     \
        x = y;                                                                 \
        y = _tmp__; } while(false)

#endif // _MAXMINCLAMP_H_
