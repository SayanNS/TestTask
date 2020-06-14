#define FORCE_SINGLE_THREADING true
#define SDTV

#ifdef SDTV

#define Yr 0.299
#define Yg 0.587
#define Yb 0.114
#define Ur -0.169
#define Ug -0.331
#define Ub 0.5
#define Vr 0.5
#define Vg -0.419
#define Vb -0.081

#endif

#ifdef HDTV

#define Yr 0.213
#define Yg 0.715
#define Yb 0.072
#define Ur -0.115
#define Ug -0.385
#define Ub 0.5
#define Vr 0.5
#define Vg -0.454
#define Vb -0.046

#endif
