#include <cstdio>
#include <cstdlib>

void test_one();
void test_two();

int main(int argc, char* argv[])
{
    test_two();
    return 0;
}

void test_two()
{
    typedef struct {
        char *a;
        short b;
        double c;
        char d;
        float e;
        char f;
        long g;
        int h;
    }rec;

    printf("sizeof(struct rec)[%d]\n", sizeof(rec));
    printf("offset of a[%d], b[%d], c[%d], e[%d], f[%d], g[%d], h[%d]\n",
            (size_t)&(((rec*)0)->a),
            (size_t)&(((rec*)0)->b),
            (size_t)&(((rec*)0)->c),
            (size_t)&(((rec*)0)->d),
            (size_t)&(((rec*)0)->e),
            (size_t)&(((rec*)0)->f),
            (size_t)&(((rec*)0)->g),
            (size_t)&(((rec*)0)->h)
        );

}

void test_one()
{
    typedef struct s
    {
        char       c;
        int        i;
        long       l;
        long long  ll;
        double     d;
    }S;

    S sss = {0};
    printf("sizeof: char[%d], int[%d], long[%d], long long[%d], double[%d], sss[%d]\n",
            sizeof(char),
            sizeof(int),
            sizeof(long),
            sizeof(long long),
            sizeof(double),
            sizeof(sss));
    printf("offset: c[%d],    i[%d],   l[%d],    ll[%d],       d[%d]\n",
            (size_t)&((S*)0)->c,
            (size_t)&((S*)0)->i,
            (size_t)&((S*)0)->l,
            (size_t)&((S*)0)->ll,
            (size_t)&((S*)0)->d,
            sizeof(sss));
}
