#include "cstdio"
#include "cstdlib"

int fibonacci(int n);

int fibonacci_memo(int n);
int fib(int n, int* memo);

int fibonacci_up(int n);

int fibonacci_up_little_space(int n);


int main(int argc, char* argv[])
{
    int ret = 0;
    int n   = 6;
    ret = fibonacci(n);
    ret = fibonacci_memo(n);
    ret = fibonacci_up(n);
    ret = fibonacci_up_little_space(n);
    //printf("fibonacci(%d) = %d\n", n, ret);
    return 0;
}

int fibonacci(int n)
{
    int ret = 0;

    if (n <=0)
        return 0;

    if (n < 3)
        return 1;

    ret = fibonacci(n - 1) + fibonacci(n - 2);
    printf("fibonacci(%d) = %d\n", n, ret);

    return ret;
}

int fibonacci_memo(int n)
{
    int* memo = new int[n + 1];

    for (int i = 0; i < n + 1; i++)
        memo[i] = -1;

    if (n <=0)
        return 0;

    return fib(n, memo); 
}
int fib(int n, int* memo)
{
    if (memo[n] != -1)
        return memo[n];
    if (n < 3)
        return 1;
    memo[n] = fib(n - 1, memo) + fib(n - 2, memo);
    printf("fibonacci_memo(%d) = %d\n", n, memo[n]);

    return memo[n];
}

int fibonacci_up(int n)
{
    if (n <= 0)
        return 0;
    if (n < 3)
        return 1;
    int* memo = new int[n + 1];

    memo[1] = 1;
    memo[2] = 1;
    for (int i = 3; i <= n; i++)
    {
        memo[i] = memo[i - 1] + memo[i - 2];
    }
    printf("fibonacci_up(%d) = %d\n", n, memo[n]);

    return memo[n];
}


int fibonacci_up_little_space(int n)
{
    if (n <= 0)
        return 0;
    if (n < 3)
        return 1;

    int memo_minus_1 = 1;
    int memo_minus_2 = 1;
    int memo         = 0;
    for (int i = 3; i <= n; i++)
    {
        memo = memo_minus_1 + memo_minus_2;
        memo_minus_2 = memo_minus_1;
        memo_minus_1 = memo;
    }
    printf("fibonacci_up_little_space(%d) = %d\n", n, memo);

    return memo;
}

