/*
    Recursive analysis of math expressions -- recursively evaluates the value of given expression.

    More complex example of recursion.
*/


#include <assert.h>
#include <stdio.h>


int p = 0;
extern const char s[];

int getE();


int getN() {
    int val = 0;

    int p0 = p;
    while ('0' <= s[p] && s[p] <= '9') {
        val = val * 10 + (s[p] - '0');   
        ++p;
    }

    assert(p0 != p);

    return val;
}

int getP() {
    int val = 0;

    if (s[p] == '(') {
        ++p;
        val = getE();

        assert(s[p] == ')');
        ++p;
        return val;
    }

    val = getN();

    return val;
}

int getT() {
    int val = getP();

    while (s[p] == '*' || s[p] == '/') {
        char op = s[p];
        ++p;
        int val2 = getP();

        if (op == '*')
            val *= val2;
        else
            val /= val2;
    }

    return val;
}

int getE() {
    int val = getT();

    while (s[p] == '-' || s[p] == '+') {
        char op = s[p];
        ++p;
        int val2 = getT();

        if (op == '-')
            val -= val2;
        else
            val += val2;
    }

    return val;
}

int getG() {
    p = 0;

    int val = getE();
    
    assert(s[p] == '\0');
    ++p;

    return val;
}


// math expression to evaluate
const char s[] = "(2+3)*7-9/(2+1)";


int main() {
    printf("%d\n", getG());
}
